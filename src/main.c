/*
 * Copyright (c) 2024 Marko Vejnovic
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <sys/_timespec.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/shell/shell.h>
#include "observer.h"
#include <zephyr/drivers/disk.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <ff.h>
#include "trutime.h"

LOG_MODULE_REGISTER(main);

OBSERVER_DECL(main_observer);
TRUTIME_DECL(time_provider);

#define DISK_DRIVE_NAME "SD"
#define DISK_MOUNT_PT "/"DISK_DRIVE_NAME":"

static FATFS fat_fs;
static struct fs_mount_t mp = {
	.type = FS_FATFS,
    .fs_data = &fat_fs,
};
#define MAX_PATH 128
static const char *disk_mount_pt = DISK_MOUNT_PT;

int main(void) {
    observer_t observer = OBSERVER_INIT(main_observer);

    LOG_INIT();
    LOG_MODULE_DECLARE(main);
    LOG_INF("Initializing modules...");

    static const char* disk_pdrv = DISK_DRIVE_NAME;
    int errno;
    if ((errno = disk_access_init(disk_pdrv)) != 0) {
        LOG_ERR("Cannot open a handle to the SDMMC device. Error: %d", errno);
        observer_set_state(observer, OBSERVER_STATE_FAIL_INIT);
    }

    uint32_t block_count;
    if ((errno = disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_COUNT,
                                   &block_count))) {
        LOG_ERR("Could not figure out how many blocks are in the device. "
                "Error: %d", errno);
    }
    LOG_INF("There are %d blocks on this device", block_count);

    uint32_t block_size;
    if ((errno = disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_SIZE,
                                   &block_size))) {
        LOG_ERR("Could not figure out the size of the block on this device. "
                "Error: %d", errno);
    }
    LOG_INF("The block size is: %d", block_size);

	mp.mnt_point = disk_mount_pt;

    if ((errno = fs_mount(&mp) != FR_OK)) {
        LOG_ERR("Could not mount the disk. Error: %d", errno);
    } else LOG_INF("Mounted the disk.");

    trutime_t time_provider = TRUTIME_INIT(time_provider);

    while (1) {
        struct tm ts;
        if ((errno = trutime_get_utc(time_provider, &ts)) != 0) {
            LOG_ERR("Could not retrieve the time.\n");
        } else {
            printk("Time: %d-%d-%d %d:%d:%d\n",
                    ts.tm_year, ts.tm_mon, ts.tm_mday,
                    ts.tm_hour, ts.tm_min, ts.tm_sec);
        }
        k_msleep(1000);
    }
}
