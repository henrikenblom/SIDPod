#include "tusb.h"
#include "diskio.h"
#include "msc_control.h"

#ifdef __cplusplus
extern "C" {
#endif

bool usbEjected = false;

#ifdef __cplusplus
}
#endif

void set_msc_ready_to_attach() {
    usbEjected = false;
}

void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {
    (void) lun;
    const char vid[] = "Enblom";
    const char pid[] = "SIDPod";
    const char rev[] = "2.0";

    strncpy((char *) vendor_id, vid, 6);
    strncpy((char *) product_id, pid, 6);
    strncpy((char *) product_rev, rev, 3);
}

#ifdef USE_SDCARD

bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    DSTATUS ds = disk_initialize(lun);
    return !(STA_NOINIT & ds) && !(STA_NODISK & ds);
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count_p, uint16_t* block_size_p) {
    if (!tud_msc_test_unit_ready_cb(lun)) {
        *block_count_p = 0;
    } else {
        DRESULT dr = disk_ioctl(lun, GET_SECTOR_COUNT, block_count_p);
        if (RES_OK != dr) *block_count_p = 0;
    }
    *block_size_p = 512;
}


bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
    (void)power_condition;
    if (load_eject) {
        if (start) {
            usbEjected = false;
        } else {
            DRESULT dr = disk_ioctl(lun, CTRL_SYNC, 0);
            if (RES_OK != dr) return false;
            usbEjected = true;
            printf("Ejected\n");
        }
    }
    return true;
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer,
                          uint32_t bufsize) {
    (void)offset;
    assert(!offset);
    assert(!(bufsize % 512));

    if (usbEjected) return -1;
    if (!tud_msc_test_unit_ready_cb(lun)) return -1;

    DRESULT dr = disk_read(lun, buffer, lba, bufsize / 512);
    if (RES_OK != dr) return -1;

    return bufsize;
}

bool tud_msc_is_writable_cb(uint8_t lun) {
    if (usbEjected) return false;

    DSTATUS ds = disk_status(lun);
    return !(STA_PROTECT & ds);
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer,
                           uint32_t bufsize) {
    (void)offset;
    assert(!offset);
    assert(!(bufsize % 512));

    if (usbEjected) return -1;
    if (!tud_msc_test_unit_ready_cb(lun)) return -1;

    DRESULT dr = disk_write(lun, buffer, lba, bufsize / 512);
    if (RES_OK != dr) return -1;

    return bufsize;
}

int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void* buffer,
                        uint16_t bufsize) {
    int32_t resplen = 0;

    switch (scsi_cmd[0]) {
        default:
                tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
        resplen = -1;
        break;
    }

    if (resplen > bufsize) resplen = bufsize;

    return resplen;
}

#else

bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    if (ejected) {
        tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3a, 0x00);
        return false;
    }
    return true;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size) {
    (void) lun;
    *block_size = FLASH_SECTOR_SIZE;
    *block_count = SECTOR_COUNT;
}

bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
    (void) lun;
    (void) power_condition;
    if (load_eject) {
        if (start) {
            ejected = false;
        } else {
            ejected = true;
        }
    }
    return true;
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize) {
    (void) offset;
    uint32_t count = bufsize / FLASH_SECTOR_SIZE;
    disk_read(lun, buffer, lba, count);
    return count * FLASH_SECTOR_SIZE;
}

bool tud_msc_is_writable_cb(uint8_t lun) {
    return !ejected;
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize) {
    (void) offset;
    uint32_t count = bufsize / FLASH_SECTOR_SIZE;
    disk_write(lun, buffer, lba, count);
    return count * FLASH_SECTOR_SIZE;
}

int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize) {
    (void) buffer;
    (void) bufsize;
    int32_t resplen = 0;
    switch (scsi_cmd[0]) {
        case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
            break;

        default:
            tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
            resplen = -1;
            break;
    }
    return resplen;
}
#endif
