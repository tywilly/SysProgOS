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
    	return false;
    }
    uint8 status_mid = __inb(ATA_ADDR_MID(bus));
    uint8 status_high = __inb(ATA_ADDR_HIGH(bus));
    if (status_mid == 0x14 && status_high == 0xeb) {
        __cio_puts("ATAPI DRIVE");
	return true;
    }
   	return false; 
}

bool _ata_bus_check(uint32 bus) {
    _reset_ata(ATA_BUS_PRIMARY);
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

void _atapi_init(void) {
    enum drive_type loc = _find_atapi_drive();
    if (loc == NO_DRIVE) {
	__cio_puts("No ATAPI FOUND"); 
        return;
    }
    uint32 drive = loc & 0xff;
    uint32 bus = loc >> 0x10;
    uint32 lba = _atapi_capacity(bus, drive);
    __cio_printf("%u",lba);
    //read_to_sector using lba
}
