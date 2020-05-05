#define __SP_KERNEL__

#include "klib.h"
#include "cio.h"
#include "optical_drive.h"
#include "x86pic.h"
#include "support.h"
#include "common.h"
#include "process.h"
#include "pci.h"

volatile uint32 ide_secondary_interrupt = 0;

void _atapi_isr_secondary(int vector, int code){
     //interrupt handles rest of pio mode get size from lba registers
     int size = ( ((unsigned short)(__inb(ATA_DATA(ATA_BUS_SECONDARY)) << 8)) | 
		     ((unsigned short)__inb(ATA_DATA(ATA_BUS_SECONDARY)) ) / 2);
     for(int i=0; i<size; i++){ 
        __cio_printf("%x",__inw(ATA_DATA(ATA_BUS_SECONDARY)));
     }
     //acknowledge interrupt so it can happen again
     __outb(PIC_MASTER_CMD_PORT,PIC_EOI);
}

uint32 _atapi_capacity_process(uint32 bus) {
    uint32 status;
    __outb(ATA_FEATURES(bus), 0); // PIO mode
    //for read capacity low and mid registers are set to 0x08
    __outb(ATA_ADDR_LOW(bus), 0x08);
    __outb(ATA_ADDR_MID(bus), 0x08);
    __outb(ATA_COMMAND(bus), ATA_PACKET_COMMAND);
    //poll on BSY
    while ((status = __inb(ATA_COMMAND(bus))) & ATA_STATUS_BUSY)
        __asm__ __volatile__ ("pause");
    __cio_printf("status_reg: %x\n", status);
    if (status & ATA_STATUS_ERROR) {
	__cio_printf("error sending packet command");
        WARNING("error sending packet command\n");
    	return 0;
    }
    __outw(ATA_DATA(bus), ATAPI_CAPACITY_READ);
    __outw(ATA_DATA(bus), 0);
    __outw(ATA_DATA(bus), 0);
    __outw(ATA_DATA(bus), 0);
    __outw(ATA_DATA(bus), 0);
    __outw(ATA_DATA(bus), 0);
    while ((status = __inb(ATA_COMMAND(bus))) & ATA_STATUS_BUSY)
        __asm__ __volatile__ ("pause");
    __cio_printf("status_reg: %x\n", status);
    if (status & ATA_STATUS_ERROR) {
	__cio_printf("error sending packet command");
        WARNING("error sending scsi command\n");
        return 0;
    }
    uint32 last_lba = __inw(ATA_DATA(bus));
    last_lba = last_lba | ((uint32) __inw(ATA_DATA(bus)) << 16);
    uint32 block_size = __inw(ATA_DATA(bus));
    block_size = block_size | ((uint32) __inw(ATA_DATA(bus)) << 16);
    return block_size;
}
int _atapi_dma_read_process(uint32 bus, uint32 drive) {
    //setup prdt
    uint32 status;
    //enable DMA
    __outb(ATA_FEATURES(bus), 1);
   // set MID and LOW to zero because of DMA usage
    __outb(ATA_ADDR_LOW(bus),0);
    __outb(ATA_ADDR_MID(bus),0);
    //identify packet command
    __outb(ATA_COMMAND(bus), ATA_PACKET_COMMAND);
    //  poll for BSY
    while ((status = __inb(ATA_COMMAND(bus))) & ATA_STATUS_BUSY)
        __asm__ __volatile__ ("pause");
    __cio_printf("status_reg: %x\n", status);
    if (status & ATA_STATUS_ERROR) {
	__cio_printf("error sending packet command");
        WARNING("error sending packet command");
        return 0;
    }
    //So this looks weird but atapi commands are sent in several outw portions to different ports to emulate
    //SCSI commands
     __outb(ATAPI_INTERRUPT_REG,ATA_DCR_HIGH_ORDER_BYTE);
    __outw(ATA_DATA(bus), ATAPI_READ_COMMAND);
    // slave bit portion.
    __outw(ATA_DATA(bus), (uint16)(drive >> 16));
    __outw(ATA_DATA(bus), (uint16)drive);
    __outw(ATA_DATA(bus), 0);
    __outw(ATA_DATA(bus), 1);
    __outw(ATA_DATA(bus), 0);
    if (status & ATA_STATUS_ERROR) {
        WARNING("error sending scsi command\n");
        return 0;
    }
    //success
    return 1;
}

int _atapi_read_process(uint32 bus, uint32 drive) {
    uint32 status;
  //  __outb(ATAPI_INTERRUPT_REG,ATA_DCR_HIGH_ORDER_BYTE);
   // uint16 size=0;
    //enable pio
    __outb(ATA_FEATURES(bus), 0);
    //Maximal byte count in this case based off of cd rom size here since this is PIO mode
    __outb(ATA_ADDR_LOW(bus), (ATAPI_SECTOR_SIZE  & 0xff));
    __outb(ATA_ADDR_HIGH(bus), (ATAPI_SECTOR_SIZE >> 8));
    //identify packet command
    __outb(ATA_COMMAND(bus), ATA_PACKET_COMMAND);
    //  poll for BSY
    while ((status = __inb(ATA_COMMAND(bus))) & ATA_STATUS_BUSY)
        __asm__ __volatile__ ("pause");
    __cio_printf("status_reg: %x\n", status);
    if (status & ATA_STATUS_ERROR) {
	__cio_printf("error sending packet command");
        WARNING("error sending packet command\n");
    	return 0;
    }
    //So this looks weird but atapi commands are sent in several outw portions to different ports to emulate
    //SCSI commands 
     __outb(ATAPI_INTERRUPT_REG,ATA_DCR_HIGH_ORDER_BYTE);
    __outw(ATA_DATA(bus), ATAPI_READ_COMMAND);
    // slave bit portion.
    __outw(ATA_DATA(bus), (uint16)(drive >> 16));
    __outw(ATA_DATA(bus), (uint16)drive);
    __outw(ATA_DATA(bus), 0);
    __outw(ATA_DATA(bus), 1);
    __outw(ATA_DATA(bus), 0);
    if (status & ATA_STATUS_ERROR) {
	__cio_printf("error sending packet command");
        WARNING("error sending scsi command\n");
    	return 0;
    }
    //success
    return 1;
}

void _atapi_init(void) {
    __install_isr(ATA_IRQ_SECONDARY + PIC_EOI, _atapi_isr_secondary);
    //Slightly confusing start up 
   // enables drive interrupts which I do not want to do until a read is executed
   // __outb(ATAPI_INTERRUPT_REG,ATA_DCR_HIGH_ORDER_BYTE);
    //sets drive select to master 0x176 IS THE register for setting which drive to use
    __outb(DRIVE_SELECT,SELECT_MASTER);
    __delay(100);
    if(__inb(CD_READY) & 0x40 ){
	__cio_puts(" ATAPI");
    }
    return;
}

void _atapi_read(void) {
   	if(_atapi_read_process(ATA_BUS_SECONDARY,SECONDARY_MASTER)) {
     	    __cio_puts( "Read Attempt:/n" );
   	}
	return;
} 

void _atapi_capacity(void) {
	__cio_printf("%x",_atapi_capacity_process(ATA_BUS_SECONDARY));
	return;
}
