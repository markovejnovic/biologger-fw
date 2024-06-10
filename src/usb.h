#ifndef USB_H
#define USB_H

#include <zephyr/usb/usbd.h>

int usb_init(struct usbd_contex** pctx);

#endif /* USB_H */
