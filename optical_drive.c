/*
** File:	optical_drive.c
**
** Author:	Michael Hopkins(mbh4480@rit.edu)
**
** Contributor:
**
** Description:	Very basic ATAPI driver for sending SCSI commands to interact with data off CD-ROMS 
*/
#define __SP_KERNEL__

#include "klib.h"
#include "cio.h"
#include "optical_drive.h"
#include "x86pic.h"
#include "support.h"
#include "common.h"
#include "process.h"
#include "pci.h"
struct prdt prdt_t[1];

//setup prdt for DMA not used and not finished
//*prdt[]  _prdt_setup(void){
//	prdt_t[0].mba = 1;
//	_pci_set_command(pci_dev->bus, pci_dev->device, pci_dev->function, 0x5);

//}

//handles interrupts for pio mode 
void _atapi_isr_secondary(int vector, int code){
     //interrupt handles rest of pio mode get size from lba registers
     int size = ( ((unsigned short)(__inb(ATA_DATA(ATA_BUS_SECONDARY)) << 8)) | 
		     ((unsigned short)__inb(ATA_DATA(ATA_BUS_SECONDARY)) ) / 2);
     for(int i=0; i<size; i++){ 
        __cio_printf("%x",__inw(ATA_DATA(ATA_BUS_SECONDARY)));
     }
     __cio_printf("\n");
     //acknowledge interrupt so it can happen again for some reason it is not firing again
     __outb(PIC_MASTER_CMD_PORT,PIC_EOI);
}

//read capacity scsi command and print out information from it
uint32 _atapi_capacity_process(uint32 bus) {
    
    __delay(100);
    uint32 status;
    __outb(ATA_FEATURES(bus), 0); // PIO mode
    //for read capacity low and mid registers are set to 0x08
    __outb(ATA_ADDR_LOW(bus), 0x08);
    __outb(ATA_ADDR_MID(bus), 0x08);
    __outb(ATA_COMMAND(bus), ATA_PACKET_COMMAND);
    //poll on BSY
    while ((status = __inb(ATA_COMMAND(bus))) & ATA_STATUS_BUSY)
        __asm__ __volatile__ ("pause");
    __cio_printf("\nstatus_reg: %x\n", status);
    if (status & ATA_STATUS_ERROR) {
	__cio_printf("error sending packet command");
    }
    __outw(ATA_DATA(bus), ATAPI_CAPACITY_READ);
    __outw(ATA_DATA(bus), 0);
    __outw(ATA_DATA(bus), 0);
    __outw(ATA_DATA(bus), 0);
    __outw(ATA_DATA(bus), 0);
    __outw(ATA_DATA(bus), 0);
    while ((status = __inb(ATA_COMMAND(bus))) & ATA_STATUS_BUSY)
        __asm__ __volatile__ ("pause");
    __cio_printf("\nstatus_reg: %x\n", status);
    if (status & ATA_STATUS_ERROR) {
	__cio_printf("error sending packet command");
        WARNING("error sending scsi command\n");
        return 0;
    }
   uint16 data1 = __inw(ATA_DATA(bus));
   uint16 data2 = __inw(ATA_DATA(bus));
   uint16 data3 = __inw(ATA_DATA(bus));
   uint16 data4 = __inw(ATA_DATA(bus));
   uint16 lba = data1 | (data2 << 8);
   uint16 block = data3 | (data4 << 8);
   uint32 capacity = (lba + 1)*block;
   __cio_printf("\nBLOCK SIZE: %x\n",block);
   __cio_printf("\nLAST LBA: %x\n",lba);
   __cio_printf("\nCAPACITY: %x\n",capacity);
   return 1;
}

//DMA version of read with DMA specific changes is not 
//currently not working failing to send scsi command I am not using at the moment
//despite the plan to due to confusion over setting data bits for DMA mode
int _atapi_dma_read_process(uint32 bus, uint32 sector) {
    //setup prdt not finished
  //  *this_prdt = prdt_setup();
  //  would output long prdt address to pci bus master 0xC to 0xF
    uint32 status;
    //send read command to bus master
    __outb(0xC8,0x8);
    //enable DMA
    __outb(ATA_FEATURES(bus), ATA_DMA_MODE);
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
    __outw(ATA_DATA(bus), 0);
    __outw(ATA_DATA(bus), sector >> 24 & 0x0f);
    __outw(ATA_DATA(bus), sector >> 16 & 0xff);
    //ADDR low and high values (as seen in pio implementation) go here in DMA mode from what I can tell
    __outw(ATA_DATA(bus), (ATAPI_SECTOR_SIZE >> 8));
    __outw(ATA_DATA(bus), (ATAPI_SECTOR_SIZE & 0xff));
    if (status & ATA_STATUS_ERROR) {
        WARNING("error sending scsi command\n");
        return 0;
    }
    //success
    //would need a case in interrupt handler to handle the buffer
    return 1;
}

//standard PIO read that actually works mostly
int _atapi_read_process(uint32 bus, uint32 sector) {
    uint32 status;
  //  __outb(ATAPI_INTERRUPT_REG,ATA_DCR_HIGH_ORDER_BYTE);
    //enable pio
    __outb(ATA_FEATURES(bus), ATA_PIO_MODE);
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
    //enable interrupts here instead to avoid inital interrupt and only grab the read interrupts
    //So this looks weird but atapi commands are sent in several outw portions to different ports to emulate
    //SCSI commands
     __outb(ATAPI_INTERRUPT_REG,ATA_DCR_HIGH_ORDER_BYTE);
    __outw(ATA_DATA(bus), ATAPI_READ_COMMAND);
    // sector portion
    __outw(ATA_DATA(bus), (uint16)(sector >> 16));
    __outw(ATA_DATA(bus), (uint16)sector);
    __outw(ATA_DATA(bus), 0);
    __outw(ATA_DATA(bus), 1);
    __outw(ATA_DATA(bus), 0);
    //check for immediate errors 
    if (status & ATA_STATUS_ERROR) {
	__cio_printf("error sending packet command");
        WARNING("error sending scsi command\n");
    	return 0;
    }
    //success
    return 1;
}

//initizilation function that installs the secondary ata irq
//selects the default drive then delays and checks status register to see if it is ready
void _atapi_init(void) {
    __install_isr(ATA_IRQ_SECONDARY + PIC_EOI, _atapi_isr_secondary);
    //Slightly confusing start up 
   // enables drive interrupts which I do not want to do until a read is executed
   // Orginally enabled interrupts here but I can just poll on BSY or DRQ  for the packet command
   // __outb(ATAPI_INTERRUPT_REG,ATA_DCR_HIGH_ORDER_BYTE);
    //sets drive select to master 0x176 IS THE register for setting which drive to use
    __outb(DRIVE_SELECT,SELECT_MASTER);
    //delay at startup for drive selection since it is not instant and needs some time before
    //it can recieve packet commands. That long init at the start is caused by this delay. 
    __delay(100);
    if(__inb(CD_READY) & 0x40 ){
	__cio_puts(" ATAPI ");
    }
    return;
}

//basic test function that calls the actual read process (PIO mode) It uses ATA_BUS_SECONDARY since 
//I had trouble detecting the default drive for qemu (which ATA_BUS_SECONDARY is) 
//it only does a read of 1 sector of the cd
void _atapi_read(void) {
   	if(_atapi_read_process(ATA_BUS_SECONDARY,1)) {
     	    __cio_puts( "Read Attempt:" );
   	}
	return;
} 

//basic test function that calls the actual read capacity
void _atapi_capacity(void) {
	if(_atapi_capacity_process(ATA_BUS_SECONDARY)) {
	    __cio_printf("\nCD inserted and Drive is correct\n");
	}
	return;
}
