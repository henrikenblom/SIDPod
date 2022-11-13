#include "tusb.h"
#include "platform_config.h"
#include "pico/unique_id.h"

#define USBD_CDC_CMD_MAX_SIZE (8)
#define USBD_CDC_IN_OUT_MAX_SIZE (64)
#define MP_STATIC_ASSERT(cond) ((void)sizeof(char[1 - 2 * !(cond)]))

void mp_usbd_port_get_serial_number(char *serial_buf) {
    pico_unique_board_id_t id;
    pico_get_unique_board_id(&id);
    // convert to hex
    int hexlen = sizeof(id.id) * 2;
    MP_STATIC_ASSERT(hexlen <= USBD_DESC_STR_MAX);
    for (int i = 0; i < hexlen; i += 2) {
        static const char *hexdig = "0123456789abcdef";
        serial_buf[i] = hexdig[id.id[i / 2] >> 4];
        serial_buf[i + 1] = hexdig[id.id[i / 2] & 0x0f];
    }
    serial_buf[hexlen] = 0;
}


const tusb_desc_device_t mp_usbd_desc_device_static = {
        .bLength = sizeof(tusb_desc_device_t),
        .bDescriptorType = TUSB_DESC_DEVICE,
        .bcdUSB = 0x0200,
        .bDeviceClass = TUSB_CLASS_MISC,
        .bDeviceSubClass = MISC_SUBCLASS_COMMON,
        .bDeviceProtocol = MISC_PROTOCOL_IAD,
        .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
        .idVendor = HW_USB_VID,
        .idProduct = HW_USB_PID,
        .bcdDevice = 0x0100,
        .iManufacturer = USBD_STR_MANUF,
        .iProduct = USBD_STR_PRODUCT,
        .iSerialNumber = USBD_STR_SERIAL,
        .bNumConfigurations = 1,
};

const uint8_t mp_usbd_desc_cfg_static[USBD_STATIC_DESC_LEN] = {
        TUD_CONFIG_DESCRIPTOR(1, USBD_ITF_STATIC_MAX, USBD_STR_0, USBD_STATIC_DESC_LEN,
                              0, USBD_MAX_POWER_MA),

#if CFG_TUD_CDC
        TUD_CDC_DESCRIPTOR(USBD_ITF_CDC, USBD_STR_CDC, USBD_CDC_EP_CMD,
                           USBD_CDC_CMD_MAX_SIZE, USBD_CDC_EP_OUT, USBD_CDC_EP_IN, USBD_CDC_IN_OUT_MAX_SIZE),
#endif
#if CFG_TUD_MSC
        TUD_MSC_DESCRIPTOR(USBD_ITF_MSC, 5, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64),
#endif
};

const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    char serial_buf[USBD_DESC_STR_MAX + 1]; // Includes terminating NUL byte
    static uint16_t desc_wstr[USBD_DESC_STR_MAX + 1]; // Includes prefix uint16_t
    const char *desc_str;
    uint16_t desc_len;

    switch (index) {
        case 0:
            desc_wstr[1] = 0x0409; // supported language is English
            desc_len = 4;
            break;
        case USBD_STR_SERIAL:
            // TODO: make a port-specific serial number callback
            mp_usbd_port_get_serial_number(serial_buf);
            desc_str = serial_buf;
            break;
        case USBD_STR_MANUF:
            desc_str = HW_USB_MANUFACTURER_STRING;
            break;
        case USBD_STR_PRODUCT:
            desc_str = HW_USB_PRODUCT_FS_STRING;
            break;
#if CFG_TUD_CDC
            case USBD_STR_CDC:
                desc_str = HW_USB_CDC_INTERFACE_STRING;
                break;
#endif
#if CFG_TUD_MSC
        case USBD_STR_MSC:
            desc_str = HW_USB_MSC_INTERFACE_STRING;
            break;
#endif
        default:
            desc_str = NULL;
    }

    if (index != 0) {
        if (desc_str == NULL) {
            return NULL; // Will STALL the endpoint
        }

        // Convert from narrow string to wide string
        desc_len = 2;
        for (int i = 0; i < USBD_DESC_STR_MAX && desc_str[i] != 0; i++) {
            desc_wstr[1 + i] = desc_str[i];
            desc_len += 2;
        }
    }
    // first byte is length (including header), second byte is string type
    desc_wstr[0] = (TUSB_DESC_STRING << 8) | desc_len;

    return desc_wstr;
}


const uint8_t *tud_descriptor_device_cb(void) {
    return (const void *) &mp_usbd_desc_device_static;
}

const uint8_t *tud_descriptor_configuration_cb(uint8_t index) {
    (void) index;
    return mp_usbd_desc_cfg_static;
}
