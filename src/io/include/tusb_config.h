#include "platform_config.h"

#ifndef HW_USB_MANUFACTURER_STRING
#define HW_USB_MANUFACTURER_STRING "Enblom"
#endif

#ifndef HW_USB_PRODUCT_FS_STRING
#define HW_USB_PRODUCT_FS_STRING "Really cool storage"
#endif

#define CFG_TUSB_RHPORT0_MODE   (OPT_MODE_DEVICE)

// MSC Configuration
#if CFG_TUD_MSC
#ifndef HW_USB_MSC_INTERFACE_STRING
#define HW_USB_MSC_INTERFACE_STRING "Board MSC"
#endif
// Set MSC EP buffer size to FatFS block size to avoid partial read/writes (offset arg).
#define CFG_TUD_MSC_BUFSIZE (FLASH_SECTOR_SIZE)
#endif

// Define static descriptor size and interface count based on the above config

#define USBD_STATIC_DESC_LEN (TUD_CONFIG_DESC_LEN +                     \
    (CFG_TUD_CDC ? (TUD_CDC_DESC_LEN) : 0) +  \
    (CFG_TUD_MSC ? (TUD_MSC_DESC_LEN) : 0)    \
    )

#define USBD_STR_0 (0x00)
#define USBD_STR_MANUF (0x01)
#define USBD_STR_PRODUCT (0x02)
#define USBD_STR_SERIAL (0x03)
#define USBD_STR_CDC (0x04)
#define USBD_STR_MSC (0x05)

#define USBD_MAX_POWER_MA (250)

#define USBD_DESC_STR_MAX (20)

#if CFG_TUD_MSC
#define USBD_ITF_MSC (0)
#define EPNUM_MSC_OUT (0x01)
#define EPNUM_MSC_IN (0x81)
#endif // CFG_TUD_MSC

/* Limits of statically defined USB interfaces, endpoints, strings */
#define USBD_ITF_STATIC_MAX (USBD_ITF_MSC + 1)
#define USBD_STR_STATIC_MAX (USBD_STR_MSC + 1)
#define USBD_EP_STATIC_MAX (EPNUM_MSC_OUT + 1)
