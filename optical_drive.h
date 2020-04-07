#ifndef OPTICAL_DRIVE
#define OPTCIAL_DRIVE

/* The default and seemingly universal sector size for CD-ROMs. */
#define ATAPI_SECTOR_SIZE 2048
 
/* The default ISA IRQ numbers of the ATA controllers. */
#define ATA_IRQ_PRIMARY 0x0E
#define ATA_IRQ_SECONDARY 0x0F

#define ATA_BUS_PRIMARY 0X1F0
#define ATA_BUS_SECONDARY 0x170

#define ATA_MASTER 0xA0
#define ATA_SLAVE 0xB0

#define ATA_PACKET_COMMAND 0xa0

#define FLOATING_BUS 0xff

enum drive_type{
    NO_DRIVE,
    PRIMARY_MASTER = 0x01f000a0,
    PRIMARY_SLAVE = 0x01f000b0,
    SECONDARY_MASTER = 0x017000a0,
    SECONDARY_SLAVE = 0x017000b0,
};

#define ATA_DATA(x)           (x)
#define ATA_FEATURES(x)      (x+1)
#define ATA_SECTOR_COUNT(x)   (x+2)
#define ATA_ADDR_LOW(x)   (x+3)
#define ATA_ADDR_MID(x)   (x+4)
#define ATA_ADDR_HIGH(x)  (x+5)
#define ATA_DRIVE_SELECT(x)   (x+6)
#define ATA_COMMAND(x)        (x+7)
#define ATA_DCR(x)        (x+0x206)

#define ATA_SELECT_DELAY(bus) \
  {__inb(ATA_DCR(bus));__inb(ATA_DCR(bus));__inb(ATA_DCR(bus));__inb(ATA_DCR(bus));}

#define ATAPI_COMMAND_IDENTIFY 0xa1
#define ATA_COMMAND_IDENTIFY 0xec

#define ATA_STATUS_BUSY 0x80
#define ATA_STATUS_READY 0x40
#define ATA_STATUS_WRITE_FAULT 0x20
#define ATA_STATUS_SEEK_COMPLETE 0x10
#define ATA_STATUS_REQUEST_READY 0x8
#define ATA_STATUS_CORRECTED_DATA 0x4
#define ATA_STATUS_INDEX 0x2
#define ATA_STATUS_ERROR 0x1

#define ATA_ERROR_BAD_SECTOR 0x80
#define ATA_ERROR_UNCORRECTABLE_DATA 0x40
#define ATA_ERROR_NO_MEDIA 0x20
#define ATA_ERROR_NO_ID 0x10
#define ATA_ERROR_NO_MEDIA2 0x08
#define ATA_ERROR_ABORTED 0x04
#define ATA_ERROR_TRACK0_NOT_FOUND 0x02
#define ATA_ERROR_ERROR 0x01

#define ATA_DCR_STOP 0x01
#define ATA_DCR_SOFTWARE_RESET 0x04
#define ATA_DCR_HIGH_ORDER_BYTE 0x80

#define ATAPI_CAPACITY_READ 0X25
#define ATAPI_PACKET 0xa0

#endif
