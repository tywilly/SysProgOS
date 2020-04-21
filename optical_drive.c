#include "klib.h"
#include "cio.h"
#include "optical_drive.h"

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
	__cio_puts("fuxking work");
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
    uint32 ide_secondary_interrupt=0;
    uint16 size=0;
    uint32 cycle=0;
    __outb(base+1, 0);
    __outb(base+4, (2048 & 0xff));
    __outb(base+5, (2048 >> 8));
    __outb(base+7, ATA_PACKET_COMMAND);
    cycle=0;
    for(int i=0; i<100; i++) {
        if( (__inb(base+7) & 0x88)==0x08 ) {
            cycle=1;
            break;
        }
    }
    if(cycle==0) { 
        return 0;
    }
    ide_secondary_interrupt=0;
    __outw(base+0, 0x25);
    __outw(base+0, (uint16)(sector >> 16));
    __outw(base+0, (uint16)sector);
    __outw(base+0, 0);
    __outw(base+0, 1);
    __outw(base+0, 0);
    cycle=0;
    for(int i=0; i<1000; i++) {
        __inb(base+7);  //wait
        if( ide_secondary_interrupt==1 ) {
            cycle=1;
            break;
        }
    }
    if(cycle==0) {  
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
   // enum drive_type loc = _find_atapi_drive();
  //  if (loc == NO_DRIVE) {
//	__cio_puts("No ATAPI FOUND"); 
  //      return;
//    }
    //select drive
    __outb(0x176, 0xE0);
    if(__inb(0x376) & 0x01) {
	__outb(0x176, 0xE0);
    }
    uint32 loc = 0x01f000a0 & 0xff;//drive
    uint32 bus = loc >> 0x10;
    //attempt to brute force capacity
    uint32 lba = _atapi_capacity(bus, PRIMARY_MASTER);
    __cio_printf("value %u" ,lba);
    //attempt to brute force a read
    if(atapi_read(PRIMARY_MASTER,1)) {
	__cio_puts( "read of 1 sector" );
    }
}
