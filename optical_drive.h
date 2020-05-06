#ifndef OPTICAL_DRIVE
#define OPTICAL_DRIVE

//prdt struct for DMA not used as did not get DMA working
struct prdt {
	uint32 offset;
	uint16 bytes; 
	uint16 mba;
};

/* The default  sector size for CD-ROMs. */
#define ATAPI_SECTOR_SIZE 2048
 
/* The default ISA IRQ number of the ATA controllers. */
#define ATA_IRQ_SECONDARY 0x0F

//Primary and Secondary buses qemu defaults on secondary form what I have seen
#define ATA_BUS_PRIMARY 0x1F0
#define ATA_BUS_SECONDARY 0x170
//drive select register for selecting desired drive
#define DRIVE_SELECT 0x176
//cd ready register to check if BSY
#define CD_READY 0x177
//interrupt register for enabling / disabling ATAPI interrupts
#define ATAPI_INTERRUPT_REG 0x376
//For drive select
#define SELECT_MASTER 0x00

#define ATA_MASTER 0xA0
#define ATA_SLAVE 0xB0

//Packet command the most used command for 
//ATAPI the packet in packet interface
#define ATA_PACKET_COMMAND 0xa0

//Drives
#define PRIMARY_MASTER 0x01f000a0
#define PRIMARY_SLAVE 0x01f000b0
#define SECONDARY_MASTER 0x017000a0
#define SECONDARY_SLAVE 0x017000b0

#define ATAPI_CAPACITY_READ 0X25
#define ATA_PIO_MODE 0
#define ATA_DMA_MODE 1
#define ATAPI_READ_COMMAND 0xa8

//ATAPI general register structure based off bus given taken from OSdevwiki
#define ATA_DATA(x)           (x)
#define ATA_FEATURES(x)      (x+1)
#define ATA_SECTOR_COUNT(x)   (x+2)
#define ATA_ADDR_LOW(x)   (x+3)
#define ATA_ADDR_MID(x)   (x+4)
#define ATA_ADDR_HIGH(x)  (x+5)
#define ATA_DRIVE_SELECT(x)   (x+6)
#define ATA_COMMAND(x)        (x+7)
#define ATA_DCR(x)        (x+0x206)

//Delay on the select drive from OSdevwiki not used at the moment
#define ATA_SELECT_DELAY(bus) \
  {__inb(ATA_DCR(bus));__inb(ATA_DCR(bus));__inb(ATA_DCR(bus));__inb(ATA_DCR(bus));}

//identify commands not used at the moment
#define ATAPI_COMMAND_IDENTIFY 0xa1
#define ATA_COMMAND_IDENTIFY 0xec

//Status Problems
#define ATA_STATUS_BUSY 0x80
#define ATA_STATUS_READY 0x40
#define ATA_STATUS_WRITE_FAULT 0x20
#define ATA_STATUS_SEEK_COMPLETE 0x10
#define ATA_STATUS_REQUEST_READY 0x8
#define ATA_STATUS_CORRECTED_DATA 0x4
#define ATA_STATUS_INDEX 0x2
#define ATA_STATUS_ERROR 0x1

//Types of errors I would have liked to implement,but didn't get the chance
//to in a more meaningful form
#define ATA_ERROR_BAD_SECTOR 0x80
#define ATA_ERROR_UNCORRECTABLE_DATA 0x40
#define ATA_ERROR_NO_MEDIA 0x20
#define ATA_ERROR_NO_ID 0x10
#define ATA_ERROR_NO_MEDIA2 0x08
#define ATA_ERROR_ABORTED 0x04
#define ATA_ERROR_TRACK0_NOT_FOUND 0x02
#define ATA_ERROR_ERROR 0x01

//DCR 
#define ATA_DCR_STOP 0x01
#define ATA_DCR_SOFTWARE_RESET 0x04
#define ATA_DCR_HIGH_ORDER_BYTE 0x80

#define ATAPI_CAPACITY_READ 0X25
#define ATAPI_PACKET 0xa0

void _atapi_capacity(void);
void _atapi_isr_secondary(int vector,int code);
void _atapi_init(void);
void _atapi_read(void);

#endif
