#include "tusb.h"
#include "platform_config.h"
#include "diskio.h"
#include "msc_control.h"

static bool ejected = false;

void set_msc_ready_to_attach() {
    ejected = false;
}

void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {
    (void) lun;
    const char vid[] = "Enblom";
    const char pid[] = "Mass Storage";
    const char rev[] = "1.0";

    strncpy((char *) vendor_id, vid, 8);
    strncpy((char *) product_id, pid, 16);
    strncpy((char *) product_rev, rev, 4);
}

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