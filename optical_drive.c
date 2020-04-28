#include "klib.h"
#include "cio.h"
#include "optical_drive.h"
#include <x86arch.h>
#include <x86pic.h>
#include <x86pit.h>

#include "common.h"
#include "klib.h"

#include "process.h"
#include "queues.h"
#include "scheduler.h"

volatile uint32 ide_secondary_interrupt = 0;
volatile uint32 ide_primary_interrupt = 0;

void _atapi_isr(void){
    __cio_printf("isr one");
    ide_primary_interrupt = 1;
}

void _atapi_isr_secondary(void){
    __cio_printf("isr two");
    ide_secondary_interrupt =1;
}
void _reset_ata(uint32 bus) {
    __outb(ATA_DCR(bus), ATA_DCR_SOFTWARE_RESET);
    ATA_SELECT_DELAY(bus);
    __outb(ATA_DCR(bus), 0);
    ATA_SELECT_DELAY(bus);
}

bool _is_atapi(uint32 bus, uint32 drive) {
    __outb(ATA_DRIVE_SELECT(bus), drive);
    ATA_SELECT_DELAY(bus);
    __outb(ATA_SECTOR_COUNT(bus), 0);
    __outb(ATA_ADDR_LOW(bus), 0);
    __outb(ATA_ADDR_MID(bus), 0);
    __outb(ATA_ADDR_HIGH(bus), 0);
    __outb(ATA_COMMAND(bus), ATA_COMMAND_IDENTIFY);
    uint8 status = __inb(ATA_COMMAND(bus));
    if (status == 0) {
	   __cio_puts("getting 0 status");
    	return false;
    }
   	return true; 
}

bool _ata_bus_check(uint32 bus) {
    _reset_ata(bus);
    uint8 ata_status = __inb(ATA_COMMAND(bus));
    return (ata_status == FLOATING_BUS);
}

enum drive_type _find_atapi_drive( void ) {
    if (_ata_bus_check(ATA_BUS_PRIMARY)) {
        if (_is_atapi(ATA_BUS_PRIMARY, ATA_MASTER))
            return PRIMARY_MASTER;
        if (_is_atapi(ATA_BUS_PRIMARY, ATA_SLAVE))
            return PRIMARY_SLAVE;
    }
    if (_ata_bus_check(ATA_BUS_SECONDARY)) {
        if (_is_atapi(ATA_BUS_SECONDARY, ATA_MASTER))
            return SECONDARY_MASTER;
        if (_is_atapi(ATA_BUS_SECONDARY, ATA_SLAVE))
            return SECONDARY_SLAVE;
    }
    return NO_DRIVE;
}

uint32 _atapi_capacity(uint32 bus, uint32 drive) {
    uint32 status;
    __outb(ATA_DRIVE_SELECT(bus), drive);
    ATA_SELECT_DELAY(bus);
    __outb(ATA_FEATURES(bus), 0); // PIO mode
    __outb(ATA_ADDR_LOW(bus), 0x08);
    __outb(ATA_ADDR_MID(bus), 0x00);
    __outb(ATA_COMMAND(bus), ATA_PACKET_COMMAND);
    while ((status = __inb(ATA_COMMAND(bus))) & ATA_STATUS_BUSY)
        __asm__ __volatile__ ("pause");
    __outw(ATA_DATA(bus), 0x0025);
    __outw(ATA_DATA(bus), 0x0000);
    __outw(ATA_DATA(bus), 0x0000);
    __outw(ATA_DATA(bus), 0x0000);
    __outw(ATA_DATA(bus), 0x0000);
    __outw(ATA_DATA(bus), 0x0000);
    while ((status = __inb(ATA_COMMAND(bus))) & ATA_STATUS_BUSY)
        __asm__ __volatile__ ("pause");
    uint32 last_lba = __inw(ATA_DATA(bus));
    last_lba = last_lba | ((uint32) __inw(ATA_DATA(bus)) << 16);
    uint32 block_size = __inw(ATA_DATA(bus));
    block_size = block_size | ((uint32) __inw(ATA_DATA(bus)) << 16);
    return last_lba;
}

//unused msf read audio cd command for later
/*int audio_cd_command(uint32 base, uint32 sector){
    __outw(base+0, 0xbf);
    __outw(base+0, (uint16)(sector >> 16));
    __outw(base+0, (uint16)sector);
    __outw(base+0, 0);
    __outw(base+0, 1);
    __outw(base+0, 0);
}
*/

int atapi_read(uint32 base, uint32 sector) {
    uint16 size=0;
    uint32 cycle=0;
    //enable pio
    __outb(base+1, ATA_PIO_MODE);
    //Maximal byte count in this case based off of cd rom size
    __outb(base+4, (ATAPI_SECTOR_SIZE  & 0xff));
    __outb(base+5, (ATAPI_SECTOR_SIZE >> 8));
    //identify packet command
    __outb(base+7, ATA_PACKET_COMMAND);
    //  cycle=0;
    //wait for drq and bsy to be clear will change to a more efficent
    // version once I know it working

    for(int i=0; i<1000; i++) {
        if( (__inb(base+7) & 0x88)== 0x08 ) {
            cycle=1;
            break;
        }
    }
    if(cycle==0) {
	__cio_printf("%02x",ide_secondary_interrupt); 
        return 0;
    }
    ide_secondary_interrupt=0;

    //So this looks weird but atapi commands are sent in several outw portions to different ports to emulate
    //scsi commands 
    __outw(base, ATAPI_READ_COMMAND);
    // slave bit portion.
    __outw(base, (uint16)(sector >> 16));
    __outw(base, (uint16)sector);
    __outw(base, 0);
    __outw(base, 1);
    __outw(base, 0);
    cycle=0;
    for(int i=0; i<1000; i++) {
        __inb(base+7);  //wait
        if( ide_secondary_interrupt==1 ) {
            cycle=1;
            break;
        }
    }
    if(cycle==0) {  
	__cio_puts("second failure ");
        return 0;
    }
    size = ( ( ((unsigned short)(__inb(base+5) << 8)) | ((unsigned short)__inb(base+4)) ) / 2);
    for(int i=0; i<size; i++) {
        __inw(base+0);
    }

    //success
    return 1;
}



void _atapi_init(void) {
    __install_isr( INT_VEC_ATAPI_PRIMARY, _atapi_isr );
    __install_isr(INT_VEC_ATAPI_SECONDARY, _atapi_isr_secondary);
    //Slightly confusing start up 
   // enables drive interrupts
    __outb(ATAPI_INTERRUPT_REG,ATA_DCR_HIGH_ORDER_BYTE);
    //sets drive select to master 0x176 IS THE register for setting which drive to use
    __outb(DRIVE_SELECT,SELECT_MASTER);
    //0X1F0 is the task register according to osdevwiki so I'm trying to bruce force it
    //pass in bus and drive
    for (int c = 1; c <= 1000; c++)
       {}
    if(__inb(CD_READY) & 0x40 ){
	__cio_puts(" cd is ready");
    }else{
	__cio_puts(" cd is not ready");	
    }
}

void _atapi_read(void) {
	__cio_puts("attempt a read");
   	if(atapi_read(ATA_BUS_SECONDARY,(ATA_BUS_SECONDARY << 4))) {
     	    __cio_puts( " read" );
   	 }
}
