#include <hardware/sync.h>
#include <memory.h>
#include "diskio.h"
#include "ff.h"
#include "platform_config.h"

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(
        BYTE pdrv,      /* Physical drive number (0..) */
        BYTE *buff,        /* Data buffer to store read data */
        LBA_t sector,    /* Sector address (LBA) */
        UINT count        /* Number of sectors to read (1..128) */
) {
    (void) pdrv;
    memcpy(buff, (void *) (FLASH_MMAP_ADDR + sector * FLASH_SECTOR_SIZE), count * FLASH_SECTOR_SIZE);
    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write(
        BYTE pdrv,          /* Physical drive nmuber (0..) */
        const BYTE *buff,    /* Data to be written */
        LBA_t sector,        /* Sector address (LBA) */
        UINT count            /* Number of sectors to write (1..128) */
) {
    (void) pdrv;

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_BASE_ADDR + sector * FLASH_SECTOR_SIZE, count * FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_BASE_ADDR + sector * FLASH_SECTOR_SIZE, buff, count * FLASH_SECTOR_SIZE);
    restore_interrupts(ints);
    return RES_OK;
}


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(
        BYTE pdrv                /* Physical drive nmuber to identify the drive */
) {
    (void) pdrv;
    return RES_OK;
}

DSTATUS disk_status(
        BYTE pdrv        /* Physical drive nmuber to identify the drive */
) {
    (void) pdrv;
    return RES_OK;
}

DRESULT disk_ioctl(
        BYTE pdrv,      /* Physical drive nmuber (0..) */
        BYTE cmd,        /* Control code */
        void *buff        /* Buffer to send/receive control data */
) {
    (void) pdrv;

    switch (cmd) {
        case CTRL_SYNC:
            return RES_OK;

        case GET_SECTOR_COUNT: {
            *((DWORD *) buff) = SECTOR_COUNT;
            return RES_OK;
        }

        case GET_SECTOR_SIZE: {
            *((DWORD *) buff) = FLASH_SECTOR_SIZE;
            return RES_OK;
        }

        case GET_BLOCK_SIZE:
            *((DWORD *) buff) = 1;
            return RES_OK;

        default:
            return RES_PARERR;
    }
}