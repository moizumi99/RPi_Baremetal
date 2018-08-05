#include "diskio.h"
#include "sdcard.h"
#include "integer.h"
#include "stdint.h"
#include "mylib.h"

static
DSTATUS Stat = STA_NOINIT;	/* Disk status */

/*--------------------------------------------------------------------------

   Public Functions defined in diskio.h (only subset)

---------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE drv			/* Drive number (always 0) */
)
{
	if (drv) return STA_NOINIT;

	return Stat;
}



DSTATUS disk_initialize (
	BYTE drv		/* Physical drive nmuber (0) */
)
{
	BYTE n, ty, cmd, buf[4];
	UINT tmr;
	DSTATUS s;


	if (drv != 0) {
        return RES_NOTRDY;
    }

    sdInitInterface();
    if (sdInitCard() != 0) {
        printf("Initialization procedure error\n");
        goto error;
    }
    if (sdHighSpeedMode() < 0) {
        printf("High speed ode transition error\n");
        goto error;
    }
    if (sdCheckSCR() < 0) {
        printf("SCR check error\n");
        goto error;
    }

    Stat = Stat & (!STA_NOINIT) & (!STA_NODISK);
    
	return Stat;

 error:
    Stat = Stat | STA_NOINIT;
    return RES_NOTRDY;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE drv,			/* Physical drive nmuber (0) */
	BYTE *buff,			/* Pointer to the data buffer to store read data */
	DWORD sector,		/* Start sector number (LBA) */
	UINT count			/* Sector count (1..128) */
)
{
	BYTE cmd;

	if (disk_status(drv) & STA_NOINIT) {
        return RES_NOTRDY;
    }
    int64_t address = sector *= 512; // sdTransferBlocks takes byte address
    
    int32_t result = sdTransferBlocks(address, count, buff, SDREAD);

    if (result) {
        return RES_ERROR;
    } else {
        return RES_OK;
    }
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE drv,			/* Physical drive nmuber (0) */
	const BYTE *buff,	/* Pointer to the data to be written */
	DWORD sector,		/* Start sector number (LBA) */
	UINT count			/* Sector count (1..128) */
)
{
	if (disk_status(drv) & STA_NOINIT) {
        return RES_NOTRDY;
    }

    /* int64_t address = sector *= 512; // sdTransferBlocks takes byte address */
    
    /* int32_t result = sdTransferBlocks(address, count, buff, SDWRITE); */

    /* if (result) { */
    /*     return RES_ERROR; */
    /* } else { */
    /*     return RES_OK; */
    /* } */
    return RES_PARERR;
}


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive nmuber (0) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	BYTE n, csd[16];
	DWORD cs;


	if (disk_status(drv) & STA_NOINIT) {
        return RES_NOTRDY;	/* Check if card is in the socket */
    }

	res = RES_ERROR;
	switch (ctrl) {
		case CTRL_SYNC :
			res = RES_OK; // current sdcard.c wait until write is complete
			break;

		case GET_SECTOR_COUNT :	/* Get number of sectors on the disk (DWORD) */
            // TODO(moiz): Make the return value correct using CMD9 and CSD
            // Right now, this always return 8GB
            *(DWORD*)buff = 8 << (30 - 9);
            res = RES_OK;
			break;

		case GET_BLOCK_SIZE :	/* Get erase block size in unit of sector (DWORD) */
			*(DWORD*)buff = 128;
			res = RES_OK;
			break;

		default:
			res = RES_PARERR;
	}

	return res;
}



