#include <kern/pci.h>
#include <inc/string.h>
#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H
#define TX_MAX         64
#define RX_MAX          128

#endif  // SOL >= 6
//以下定义来自  QEMU's e1000_hw.h
#define E1000_TCTL     0x00400  /* TX Control - RW */
#define E1000_TDBAL    0x03800  /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH    0x03804  /* TX Descriptor Base Address High - RW */
#define E1000_TDLEN    0x03808  /* TX Descriptor Length - RW */
#define E1000_TDH      0x03810  /* TX Descriptor Head - RW */
#define E1000_TDT      0x03818  /* TX Descripotr Tail - RW */
#define E1000_TIPG     0x00410  /* TX Inter-packet gap -RW */
#define E1000_TCTL_EN            0x00000002    /* enable tx */
#define E1000_TCTL_BCE           0x00000004    /* busy check enable */
#define E1000_TCTL_PSP           0x00000008    /* pad short packets */
#define E1000_TCTL_CT            0x00000ff0    /* collision threshold */
#define E1000_TCTL_COLD          0x003ff000    /* collision distance */

#define E1000_TXD_CMD_RS         0x08000000     /* Report Status */
#define E1000_TXD_CMD_EOP    0x01000000 /* End of Packet */

#define E1000_TXD_STAT_DD        0x00000001     /* Descriptor Done */

#define E1000_STATUS   0x00008  /* Device Status - RO */
#define e1000_print_status(offset) \
        cprintf("the E1000 status register: [%08x]\n", *(pci_e1000+(offset>>2)));


#define E1000_RCTL_EN             0x00000002    /* enable */
#define E1000_RCTL_SBP            0x00000004    /* store bad packet */
#define E1000_RCTL_UPE            0x00000008    /* unicast promiscuous enable */
#define E1000_RCTL_MPE            0x00000010    /* multicast promiscuous enab */
#define E1000_RCTL_LPE            0x00000020    /* long packet enable */
#define E1000_RCTL_LBM_NO         0x00000000    /* no loopback mode */
#define E1000_RCTL_BAM            0x00008000    /* broadcast enable */
#define E1000_RCTL_SZ_2048        0x00000000    /* rx buffer size 2048 */
#define E1000_RCTL_SECRC          0x04000000    /* Strip Ethernet CRC */
#define E1000_RXD_STAT_DD       0x01    /* Descriptor Done */
#define E1000_RXD_STAT_EOP      0x02    /* End of Packet */
#define E1000_RCTL     0x00100  /* RX Control - RW */
#define E1000_RDBAL    0x02800  /* RX Descriptor Base Address Low - RW */
#define E1000_RDBAH    0x02804  /* RX Descriptor Base Address High - RW */
#define E1000_RDLEN    0x02808  /* RX Descriptor Length - RW */
#define E1000_RDH      0x02810  /* RX Descriptor Head - RW */
#define E1000_RDT      0x02818  /* RX Descriptor Tail - RW */

#define E1000_MTA      0x05200  /* Multicast Table Array - RW Array */
#define E1000_RA       0x05400  /* Receive Address - RW Array */
#define E1000_RAH_AV  0x80000000        /* Receive descriptor valid */

// 由于pci_e1000是uint32_t的，如果直接加offset，就相当于加了offset*sizeof(uint32_t)
// 例如the E1000:[ef804000] offset:[00000008] sum:[ef804020]#pragma once
#define TX_MAX         64
//传输描述符
struct tx_desc
{
    uint64_t addr;   //Address of the transmit descriptor in the host memory
    uint16_t length; //The Checksum offset field indicates where to insert a TCP checksum if this mode is enabled. 
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css; //The Checksum start field (TDESC.CSS) indicates where to begin computing the checksum. 
    uint16_t special;
}__attribute__((packed));//告诉编译器取消结构在编译过程中的优化对齐,按照实际占用字节数进行对齐，

//传输描述符数组
struct tx_desc tx_list[TX_MAX];

//接收描述符
struct rx_desc
{
	uint64_t addr;
	uint16_t length;
	uint16_t pcs; //Packet Checksum
	uint8_t status;
	uint8_t errors;
	uint16_t special;
}__attribute__((packed));
//接收描述符数组
struct rx_desc rx_list[RX_MAX];
//包缓冲区 
//以太网数据包的最大大小为1518字节，将其限制在2048方便对齐
struct packets {
    char buffer[2048];
}__attribute__((packed));
//缓冲区数组
struct packets tx_buf[TX_MAX];
struct packets rx_buf[RX_MAX];


int e1000_init(struct pci_func* pcif);


int e1000_transmit(void* addr, int length);
int e1000_receive(void* addr);
void e1000_transmit_init();
void e1000_receive_init();

volatile uint32_t* pci_e1000;


