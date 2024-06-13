#include "usb.h"
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/usb/usbd.h>

LOG_MODULE_REGISTER(usb);

// These flags are normally injected from Kconfig but
// CONFIG_USB_DEVICE_STACK_NEXT and CONFIG_USB_DEVICE_STACK are currently
// mutually exclusive. They are, therefore, explicitly specified here.
#define CONFIG_USB_DEVICE_VID 0x1d50
#define CONFIG_USB_DEVICE_PID 0x1
#define CONFIG_USB_DEVICE_MANUFACTURER "DeRosa"
#define CONFIG_USB_DEVICE_PRODUCT "FLoggy"
#define CONFIG_USB_MAX_POWER 50  // * 10 mA

USBD_DEVICE_DEFINE(usb_device, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
                   CONFIG_USB_DEVICE_VID, CONFIG_USB_DEVICE_PID);
USBD_DESC_LANG_DEFINE(default_lang);
USBD_DESC_MANUFACTURER_DEFINE(default_mfr, CONFIG_USB_DEVICE_MANUFACTURER);
USBD_DESC_PRODUCT_DEFINE(default_product, CONFIG_USB_DEVICE_PRODUCT);
USBD_DESC_SERIAL_NUMBER_DEFINE(default_sn);

static const uint8_t attributes = (IS_ENABLED(CONFIG_SAMPLE_USBD_SELF_POWERED) ?
				   USB_SCD_SELF_POWERED : 0) |
				  (IS_ENABLED(CONFIG_SAMPLE_USBD_REMOTE_WAKEUP) ?
				   USB_SCD_REMOTE_WAKEUP : 0);

USBD_CONFIGURATION_DEFINE(default_fs_config, attributes, CONFIG_USB_MAX_POWER);
USBD_CONFIGURATION_DEFINE(default_hs_config, attributes, CONFIG_USB_MAX_POWER);

static int register_fs_classes(struct usbd_contex *uds_ctx)
{
	int err = 0;

	STRUCT_SECTION_FOREACH_ALTERNATE(usbd_class_fs, usbd_class_node, c_nd) {
		/* Pull everything that is enabled in our configuration. */
		err = usbd_register_class(uds_ctx, c_nd->c_data->name,
					  USBD_SPEED_FS, 1);
		if (err) {
			LOG_ERR("Failed to register FS %s (%d)",
				c_nd->c_data->name, err);
			return err;
		}

		LOG_DBG("Register FS %s", c_nd->c_data->name);
	}

	return err;
}

static int register_hs_classes(struct usbd_contex *uds_ctx)
{
	int err = 0;

	STRUCT_SECTION_FOREACH_ALTERNATE(usbd_class_hs, usbd_class_node, c_nd) {
		/* Pull everything that is enabled in our configuration. */
		err = usbd_register_class(uds_ctx, c_nd->c_data->name,
					  USBD_SPEED_HS, 1);
		if (err) {
			LOG_ERR("Failed to register HS %s (%d)",
				c_nd->c_data->name, err);
			return err;
		}

		LOG_DBG("Register HS %s", c_nd->c_data->name);
	}

	return err;
}

int add_configuration(struct usbd_contex* ctx,
                      enum usbd_speed speed,
                      struct usbd_config_node* cfg) {
    int err;

    if ((err = usbd_add_configuration(ctx, speed, cfg)) != 0) {
        LOG_ERR("Failed to add the configuration (%d).", err);
        return err;
    }

    if ((err = speed == USBD_SPEED_FS
             ? register_fs_classes(ctx)
             : register_hs_classes(ctx)) != 0) {
        LOG_ERR("Failed to register the USB classes (%d).", err);
        return err;
    }

    return 0;
}

int usb_init(struct usbd_contex** pctx) {
    int err = -1;
    *pctx = NULL;

    if ((err = usbd_add_descriptor(&usb_device, &default_lang)) != 0) {
        LOG_ERR("Failed to initialize the USB language (%d)", err);
        return err;
    }

    if ((err = usbd_add_descriptor(&usb_device, &default_mfr)) != 0) {
        LOG_ERR("Failed to initialize the USB manufacturer (%d)", err);
        return err;
    }

    if ((err = usbd_add_descriptor(&usb_device, &default_product)) != 0) {
        LOG_ERR("Failed to initialize the USB product (%d)", err);
        return err;
    }

    if ((err = usbd_add_descriptor(&usb_device, &default_sn)) != 0) {
        LOG_ERR("Failed to initialize the USB SN (%d)", err);
        return err;
    }

    const enum usbd_speed target_speed = USBD_SPEED_FS;
        //usbd_caps_speed(&usb_device) >= USBD_SPEED_HS
        //    ? USBD_SPEED_HS
        //    : USBD_SPEED_FS;
    if ((err = add_configuration(&usb_device, target_speed,
                                 target_speed == USBD_SPEED_HS
                                     ? &default_hs_config
                                     : &default_fs_config)) != 0) {
        LOG_ERR("Could not figure out the USB configuration (%d).", err);
        return err;
    }

    if ((err = usbd_device_set_bcd(&usb_device, target_speed, 0x0200)) != 0) {
        LOG_ERR("Failed to add USB 2.0 extension descriptor.");
        return err;
    }

    if ((err = usbd_init(&usb_device)) != 0) {
        LOG_ERR("Failed to initialize the USB (%d).", err);
        return err;
    }

    *pctx = &usb_device;
    return err;
}
