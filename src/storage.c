#include "storage.h"
#include "thread_specs.h"
#include <zephyr/usb/usbd.h>
#include <stdint.h>
#include <zephyr/kernel/thread.h>
#include <ff.h>
#include <sys/errno.h>
#include <zephyr/drivers/disk.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/sys/slist.h>
#include "usb.h"
#include <zephyr/usb/class/usbd_msc.h>
#include <zephyr/usb/usb_device.h>

#define MAX_PATH 256
#define DISK_NAME "SD"
#define DISK_MOUNT_POINT "/"DISK_NAME":"
#define MIN_DISK_SIZE_MB 1024
#define SECTOR_MAX 2048

LOG_MODULE_REGISTER(storage);

struct usbd_contex* usb_device;
USBD_DEFINE_MSC_LUN(SD, "Zephyr", DISK_NAME, "0.00");

struct storage {
    size_t open_objects;
    const char* disk_name;
    struct fs_mount_t mount_point;
    FATFS fat_fs;

    observer_t observer;

    struct {
        uint32_t sz;
        uint32_t count;

        uint64_t disk_sz_mb;
    } block;
};

struct experiment {
    storage_t storage;
};

enum block_device_status {
    BLOCK_DEVICE_STATUS_APPEARS_SENSIBLE,
    BLOCK_DEVICE_STATUS_NO_OR_BAD_DISK,
    BLOCK_DEVICE_STATUS_NO_SPACE_ON_DISK,
    BLOCK_DEVICE_STATUS_NO_OR_CORRUPT_FAT,
    BLOCK_DEVICE_STATUS_NO_SPACE_ON_FAT,
};

static bool sdcard_got_dced(storage_t storage) {
    // As discussed in storage_init, we choose to blindly assume the card is
    // reachable if the sector size is currently unknown. This is safe to do
    // out of two reasons -- other fallbacks MUST cover the case of no card
    // being in in the first place.
    if (storage->block.sz == UINT32_MAX) {
        return false;
    }

    uint8_t* _data = k_malloc(storage->block.sz);
    int res = disk_access_read(DISK_NAME, _data, 2048, 1) != 0;
    k_free(_data);
    return res;
}

static int storage_get_status(storage_t storage,
                              enum block_device_status* status,
                              uint64_t* extra_status) {
    *status = -1;

    if (storage == NULL) {
        return -EINVAL;
    }

    int errnum;

    if ((errnum = disk_access_status(DISK_NAME)) != DISK_STATUS_OK
        || sdcard_got_dced(storage)) {
        LOG_ERR("Querying the status of %s failed. Error: %d",
                DISK_NAME, errnum);
        *status = BLOCK_DEVICE_STATUS_NO_OR_BAD_DISK;
        return -ENOTBLK; // Cannot trust DISK_STATUS_OK to be zero.
    }

    if ((errnum = disk_access_ioctl(DISK_NAME, DISK_IOCTL_GET_SECTOR_COUNT,
                                   &storage->block.count))) {
        LOG_ERR("Could not figure out how many blocks are in the device. "
                "Error: %d", errnum);
        return errnum;
    }
    LOG_DBG("There are %d blocks on this device", storage->block.count);

    if ((errnum = disk_access_ioctl(DISK_NAME, DISK_IOCTL_GET_SECTOR_SIZE,
                                   &storage->block.sz))) {
        LOG_ERR("Could not figure out the size of the block on this device. "
                "Error: %d", errnum);
        return errnum;
    }


    storage->block.disk_sz_mb =
        (uint64_t)storage->block.sz * (uint64_t)storage->block.count
        / 1024ull / 1024ull;
    LOG_DBG("The disk size is: %llu MB", storage->block.disk_sz_mb);

    if (storage->block.disk_sz_mb < MIN_DISK_SIZE_MB) {
        LOG_ERR("The disk is %llu MB which is less than the min %d MB.",
                storage->block.disk_sz_mb, MIN_DISK_SIZE_MB);
        *status = BLOCK_DEVICE_STATUS_NO_SPACE_ON_DISK;
        *extra_status = storage->block.disk_sz_mb;
        return -ENOMEM;
    }

    struct fs_statvfs filesystem_stats;
    if ((errnum = fs_statvfs(storage->mount_point.mnt_point,
                            &filesystem_stats)) != FR_OK) {
        LOG_ERR("Could not query information on %s.",
                storage->mount_point.mnt_point);
        *status = BLOCK_DEVICE_STATUS_NO_OR_CORRUPT_FAT;
        return errnum;
    }

    const uint64_t space_left_mb =
        (uint64_t)filesystem_stats.f_bfree * (uint64_t)filesystem_stats.f_frsize
        / 1024ull / 1024ull;
    if (space_left_mb < MIN_DISK_SIZE_MB) {
        LOG_ERR("The FAT parition has GB %llu free. Less than the min %d GB.",
                space_left_mb, MIN_DISK_SIZE_MB);
        *status = BLOCK_DEVICE_STATUS_NO_SPACE_ON_FAT;
        return -ENOMEM;
    }

    *status = BLOCK_DEVICE_STATUS_APPEARS_SENSIBLE;
    return 0;
}

int storage_close(storage_t storage) {
    if (storage == NULL) {
        return 0;
    }

    if (storage->open_objects != 0) {
        return -EMFILE;
    }

    k_free(storage);
    return 0;
}

stored_experiment_t storage_stored_experiment_new(storage_t storage) {
    if (storage == NULL) {
        return NULL;
    }

    storage->open_objects++;

    stored_experiment_t experiment = k_malloc(sizeof(struct experiment));
    experiment->storage = storage;

    return experiment;
}

int storage_stored_experiment_close(stored_experiment_t experiment) {
    if (experiment == NULL) {
        return 0;
    }
    
    experiment->storage--;

    k_free(experiment);
    return 0;
}

K_THREAD_STACK_DEFINE(management_thread_stack,
                      THREAD_BLOCK_STORAGE_MANAGEMENT_STACK_SIZE);
static struct k_thread management_thread_data;

// Cheap hack. Should be a separate module, but this architecture is all a bit
// of a hack.
static bool usb_enabled = false;

static void management_thread_runnable(void* p0, void* p1, void* p2) {
    LOG_INF("Starting to manage storage...");

    storage_t storage = p0;

    // Preferring while-true over deadline scheduler for convenience. Totally
    // okay poor performance here.
    while (true) {
        const uint32_t start = k_uptime_get_32();

        enum block_device_status status;
        uint64_t extra_status;

        const size_t MAX_STATUS_UPDATE_COUNT = 3;
        size_t status_update_count = 0;
        query_status: {
            int errnum = storage_get_status(storage, &status, &extra_status);
            switch (status) {
                case BLOCK_DEVICE_STATUS_APPEARS_SENSIBLE:
                    LOG_DBG("It appears the disk is operating normally.");
                    if (!usb_enabled) {
                        if ((errnum = usb_enable(NULL)) != 0) {
                            LOG_ERR("Could not initialize the USB (%d)",
                                    errnum);
                            usb_enabled = false;
                        } else {
                            LOG_INF("Initialized the USB module.");
                            usb_enabled = true;
                        }
                    }
                    observer_flag_lower(storage->observer,
                                        OBESRVER_FLAG_NO_DISK);
                    break;
                case BLOCK_DEVICE_STATUS_NO_OR_BAD_DISK:
                    LOG_INF("It appears the disk is unavailable / corrupt.");
                    observer_flag_raise(storage->observer,
                                        OBESRVER_FLAG_NO_DISK);

                    // TODO(markovejnovic): Here would be a good spot to
                    // attempt to reinitialize the SD card, however there is no
                    // support for that at the moment.

                    break;
                case BLOCK_DEVICE_STATUS_NO_SPACE_ON_DISK:
                    LOG_INF("It appears the disk is too small.");
                    observer_flag_raise(storage->observer,
                                        OBESRVER_FLAG_NO_DISK);
                    break;
                case BLOCK_DEVICE_STATUS_NO_OR_CORRUPT_FAT:
                    LOG_INF("It appears the disk does not have a good FAT "
                            "partition.");
                    // In this case we can also try our best to remount the FAT
                    // device in hopes it will come up. The block device exists
                    // for sure, so we can give this a shot. Let's not flag
                    // this as a fault just yet...
                    if ((errnum = fs_mount(&storage->mount_point)) != FR_OK) {
                        LOG_ERR("Could not mount the disk %s at %s. Error: %d",
                                storage->disk_name,
                                storage->mount_point.mnt_point, errnum);
                        // Well we tried and failed, that doesn't look good --
                        // mark it as gonezo.
                        observer_flag_raise(storage->observer,
                                            OBESRVER_FLAG_NO_DISK);
                    } else {
                        // We mounted the disk! In order to avoid waiting for
                        // the next cycle, let's immediately re-run this
                        // procedure.
                        LOG_INF("Successfully mounted an SD card.");
                        status_update_count++;
                        if (status_update_count < MAX_STATUS_UPDATE_COUNT) {
                            goto query_status;
                        }
                    }
                    break;
                case BLOCK_DEVICE_STATUS_NO_SPACE_ON_FAT:
                    LOG_INF("It appears the FAT partition is too small.");
                    observer_flag_raise(storage->observer,
                                        OBESRVER_FLAG_NO_DISK);
                    break;
                default:
                    LOG_ERR("Received an unreasonable and unexpected value "
                            "from storage_get_status: %d", status);
                    // Not the user's fault -- let's not confuse them.
                    observer_flag_lower(storage->observer,
                                        OBESRVER_FLAG_NO_DISK);
                    break;
            }
        }

        const uint32_t stop = k_uptime_get_32();
        k_msleep(THREAD_BLOCK_STORAGE_MANAGEMENT_PERIOD_MS - (stop - start));
    }
}

storage_t storage_init(observer_t observer) {
    LOG_INF("Initializing storage...");
    storage_t storage = k_malloc(sizeof(struct storage));

    *storage = (struct storage){
        .open_objects = 0,
        .disk_name = DISK_NAME,
        .mount_point = (struct fs_mount_t){
            .type = FS_FATFS,
            .fs_data = &storage->fat_fs,
            .mnt_point = DISK_MOUNT_POINT,
        },
        .observer = observer,
        .block = {
            // UINT32_MAX max indicates that the block size has not been
            // initialized. In this case, we shall assume the SD card is
            // reachable as actually performing a read requires an extremely
            // expensive memory allocation, possibly up to 8K, depending on the
            // sector size.
            .sz = UINT32_MAX,
        },
    };

    int errnum;

    if ((errnum = disk_access_init(DISK_NAME)) != 0) {
        LOG_ERR("Cannot open a handle to the SDMMC device. Error: %d", errnum);
        goto exit_fault;
    }

    // This module shall start a management thread that will manage the
    // storage. We do not need this to be extremely zealous, just need to know
    // some basics on the device.
    LOG_DBG("Beginning SD card management thread...");
    k_thread_create(
        &management_thread_data,
        management_thread_stack,
        K_THREAD_STACK_SIZEOF(management_thread_stack),
        management_thread_runnable, storage, NULL, NULL,
        THREAD_BLOCK_STORAGE_MANAGEMENT_PRIORITY, 0, K_NO_WAIT
    );

    return storage;

exit_fault:
    k_free(storage);
    return NULL;
}

