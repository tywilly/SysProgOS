/*
  File: e1000.h

  Author: Oscar Lowry

  Contributor:

  Description: Definitions for the e1000 network driver

 */


/* Identification of Device */
#define CLASS_NETCTL 0x02
#define CLASS_ETHRNT 0x00
#define E1000_VNDID  0x8086
#define E1000_DEVID  0x100E

// Values taken from another hobyist
// Rather than one by one from the manual

/* Registers */
#define E1000_CTL      (0x0000)  /* Device Control Register - RW */
#define E1000_EERD     (0x0014)  /* EEPROM Read - RW */
#define E1000_ICR      (0x00C0)  /* Interrupt Cause Read - R */
#define E1000_IMS      (0x00D0)  /* Interrupt Mask Set - RW */
#define E1000_IMC      (0x00D8)  /* Interrupt Mask Clear - RW */
#define E1000_RCTL     (0x0100)  /* RX Control - RW */
#define E1000_TCTL     (0x0400)  /* TX Control - RW */
#define E1000_TIPG     (0x0410)  /* TX Inter-packet gap -RW */
#define E1000_RDBAL    (0x2800)  /* RX Descriptor Base Address Low - RW */
#define E1000_RDBAH    (0x2804)  /* RX Descriptor Base Address High - RW */
#define E1000_RDTR     (0x2820)  /* RX Delay Timer */
#define E1000_RADV     (0x282C)  /* RX Interrupt Absolute Delay Timer */
#define E1000_RDH      (0x2810)  /* RX Descriptor Head - RW */
#define E1000_RDT      (0x2818)  /* RX Descriptor Tail - RW */
#define E1000_RDLEN    (0x2808)  /* RX Descriptor Length - RW */
#define E1000_RSRPD    (0x2C00)  /* RX Small Packet Detect Interrupt */
#define E1000_TDBAL    (0x3800)  /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH    (0x3804)  /* TX Descriptor Base Address Hi - RW */
#define E1000_TDLEN    (0x3808)  /* TX Descriptor Length - RW */
#define E1000_TDH      (0x3810)  /* TX Descriptor Head - RW */
#define E1000_TDT      (0x3818)  /* TX Descripotr Tail - RW */
#define E1000_MTA      (0x5200)  /* Multicast Table Array - RW Array */
#define E1000_RA       (0x5400)  /* Receive Address - RW Array */

/* Device Control */
#define E1000_CTL_SLU     0x00000040    /* set link up */
#define E1000_CTL_FRCSPD  0x00000800    /* force speed */
#define E1000_CTL_FRCDPLX 0x00001000    /* force duplex */
#define E1000_CTL_RST     0x00400000    /* full reset */

/* EEPROM */
#define E1000_EERD_ADDR 8 /* num of bit shifts to get to addr section */
#define E1000_EERD_DATA 16 /* num of bit shifts to get to data section */
#define E1000_EERD_READ (1 << 0) /* 0th bit */
#define E1000_EERD_DONE (1 << 4) /* 4th bit */

/* Transmit Control */
#define E1000_TCTL_RST    0x00000001    /* software reset */
#define E1000_TCTL_EN     0x00000002    /* enable tx */
#define E1000_TCTL_BCE    0x00000004    /* busy check enable */
#define E1000_TCTL_PSP    0x00000008    /* pad short packets */
#define E1000_TCTL_CT     0x00000ff0    /* collision threshold */
#define E1000_TCTL_CT_SHIFT 4
#define E1000_TCTL_COLD   0x003ff000    /* collision distance */
#define E1000_TCTL_COLD_SHIFT 12
#define E1000_TCTL_SWXOFF 0x00400000    /* SW Xoff transmission */
#define E1000_TCTL_PBE    0x00800000    /* Packet Burst Enable */
#define E1000_TCTL_RTLC   0x01000000    /* Re-transmit on late collision */
#define E1000_TCTL_NRTU   0x02000000    /* No Re-transmit on underrun */
#define E1000_TCTL_MULR   0x10000000    /* Multiple request support */

/* Transmit Descriptor command definitions [E1000 3.3.3.1] */
#define E1000_TXD_CMD_EOP    0x01 /* End of Packet */
#define E1000_TXD_CMD_IFCS   0x02 /* Insert FCS (Ethernet CRC) */
#define E1000_TXD_CMD_RS     0x08 /* Report Status */
#define E1000_TXD_CMD_DEXT   0x20 /* Descriptor extension (0 = legacy) */

/* Transmit Descriptor status definitions [E1000 3.3.3.2] */
#define E1000_TXD_STAT_DD    0x00000001 /* Descriptor Done */

#define ETHERNET_ADDR_LEN   6

unsigned int _e1000_read_mmio(uint16 reg);

void _e1000_write_mmio(uint16 reg, uint32 val);

uint16 _e1000_eeprom_read(uint8 addr);

void _e1000_dump_MAC( void );

int _e1000_tx(uint8 *data, int len);

int _e1000_ethernet_frame(uint8 *data, int len, uint16 type);

int _e1000_init( void );
