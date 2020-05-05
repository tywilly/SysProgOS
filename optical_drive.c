#include "klib.h"
#include "cio.h"
#include "optical_drive.h"
#include "x86pic.h"
#include "support.h"
#include "common.h"

#include "process.h"

volatile uint32 ide_secondary_interrupt = 0;

void _atapi_isr_secondary(int vector, int code){

  //  uint32 base = ATA_BUS_SECONDARY;
    __cio_printf("isr two");
     int size = ( ((unsigned short)(__inb(ATA_DATA(ATA_BUS_SECONDARY)) << 8)) | ((unsigned short)__inb(ATA_DATA(ATA_BUS_SECONDARY)) ) / 2);
     for(int i=0; i<size; i++){ 
        __cio_printf("%20x",__inw(ATA_DATA(ATA_BUS_SECONDARY)));
     }
     __outb(PIC_MASTER_CMD_PORT,PIC_EOI);

}

uint32 _atapi_capacity_process(uint32 bus) {
    uint32 status;
   // __outb(ATA_DRIVE_SELECT(bus), drive);
   // ATA_SELECT_DELAY(bus);
    __outb(ATA_FEATURES(bus), 0); // PIO mode
    __outb(ATA_ADDR_LOW(bus), 0x08);
    __outb(ATA_ADDR_MID(bus), 0x08);
    __outb(ATA_COMMAND(bus), ATA_PACKET_COMMAND);
    while ((status = __inb(ATA_COMMAND(bus))) & ATA_STATUS_BUSY)
        __asm__ __volatile__ ("pause");
    __cio_printf("status1: %x\n", status);
    if (status & ATA_STATUS_ERROR)
        __cio_printf("error sending packet command\n");
    __outw(ATA_DATA(bus), 0x0025);
    __outw(ATA_DATA(bus), 0);
    __outw(ATA_DATA(bus), 0);
    __outw(ATA_DATA(bus), 0);
    __outw(ATA_DATA(bus), 0);
    __outw(ATA_DATA(bus), 0);
    while ((status = __inb(ATA_COMMAND(bus))) & ATA_STATUS_BUSY)
        __asm__ __volatile__ ("pause");
    __cio_printf("status1: %x\n", status);
    if (status & ATA_STATUS_ERROR)
        __cio_printf("error sending scsi command\n");
    uint32 last_lba = __inw(ATA_DATA(bus));
    last_lba = last_lba | ((uint32) __inw(ATA_DATA(bus)) << 16);
    uint32 block_size = __inw(ATA_DATA(bus));
    block_size = block_size | ((uint32) __inw(ATA_DATA(bus)) << 16);
    return block_size;
}

int _atapi_read_process(uint32 bus, uint32 sector) {
    uint32 status;
  //  __outb(ATAPI_INTERRUPT_REG,ATA_DCR_HIGH_ORDER_BYTE);
   // uint16 size=0;
    //enable pio
    __outb(ATA_FEATURES(bus), 0);
    //Maximal byte count in this case based off of cd rom size
    __outb(bus+4, (ATAPI_SECTOR_SIZE  & 0xff));
    __outb(bus+5, (ATAPI_SECTOR_SIZE >> 8));
   // set MID and LOW to zero because of DMA usage
 //   __outb(ATA_ADDR_LOW(bus),0);
  //  __outb(ATA_ADDR_MID(bus),0);
    //identify packet command
    __outb(ATA_COMMAND(bus), ATA_PACKET_COMMAND);
    //  cycle=0;
    //wait for drq and bsy to be clear will change to a more efficent
    // version once I know it working
    while ((status = __inb(ATA_COMMAND(bus))) & ATA_STATUS_BUSY)
        __asm__ __volatile__ ("pause");
    __cio_printf("status1: %x\n", status);
    if (status & ATA_STATUS_ERROR)
        __cio_printf("error sending packet command\n");
  //  ide_secondary_interrupt=0;

    //So this looks weird but atapi commands are sent in several outw portions to different ports to emulate
    //scsi commands 
     __outb(ATAPI_INTERRUPT_REG,ATA_DCR_HIGH_ORDER_BYTE);
    __outw(ATA_DATA(bus), ATAPI_READ_COMMAND);
    // slave bit portion.
    __outw(ATA_DATA(bus), (uint16)(sector >> 16));
    __outw(ATA_DATA(bus), (uint16)sector);
    __outw(ATA_DATA(bus), 0);
    __outw(ATA_DATA(bus), 1);
    __outw(ATA_DATA(bus), 0);
   // while ((status = __inb(ATA_COMMAND(bus))) & ATA_STATUS_BUSY)
     //   __asm__ __volatile__ ("pause");
   // __cio_printf("status1: %x\n", status);
    if (status & ATA_STATUS_ERROR)
        __cio_printf("error sending scsi command\n");
   // cycle=0;
  //  for(int i=0; i<1000; i++) {
    //    __inb(base+7);  //wait
      //  if( ide_secondary_interrupt==1 ) {
        //    cycle=1;
          //  break;
      //  }
   // }
  //  if(cycle==0) {  
//	__cio_puts("second failure ");
     //   return 0;
   // }
   // size = ( ( ((unsigned short)(__inb(base+5) << 8)) | ((unsigned short)__inb(base+4)) ) / 2);
   // for(int i=0; i<size; i++) {
   //     __inw(base+0);
   // }

    //success
    return 1;
}



void _atapi_init(void) {
    __install_isr(ATA_IRQ_SECONDARY + PIC_EOI, _atapi_isr_secondary);
    //Slightly confusing start up 
   // enables drive interrupts
   // __outb(ATAPI_INTERRUPT_REG,ATA_DCR_HIGH_ORDER_BYTE);
    //sets drive select to master 0x176 IS THE register for setting which drive to use
    __outb(DRIVE_SELECT,SELECT_MASTER);
    //0X1F0 is the task register according to osdevwiki so I'm trying to bruce force it
    //pass in bus and drive
    for (int c = 1; c <= 1000; c++)
       {}
    if(__inb(CD_READY) & 0x40 ){
	__cio_puts("ATAPI");
    }
    return;
}

void _atapi_read(void) {
	__cio_puts("attempt a read");
   	if(_atapi_read_process(ATA_BUS_SECONDARY,1)) {
     	    __cio_puts( " read" );
   	}
	return;
} 

void _atapi_capacity(void) {
	__cio_puts("attempt capacity");
	__cio_printf("%x this is it",_atapi_capacity_process(ATA_BUS_SECONDARY));
	return;
}
