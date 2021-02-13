#include <kern/e1000.h>
#include <kern/pmap.h>

// LAB 6: Your driver code here

int
e1000_init(struct pci_func* pcif)
{
	pci_func_enable(pcif);
	//pci_e1000 是1个指针，指向映射地址， uint32_t* pci_e1000;
	//他创建了一个虚拟内存映射
	pci_e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
	e1000_print_status(E1000_STATUS);
	
	e1000_transmit_init();
	e1000_receive_init();

	return 1;
}

int
e1000_transmit(void* addr, int length) {
	//TDT是传输描述符数组的索引
	int tail = pci_e1000[E1000_TDT >> 2];
	//得到下一个描述符
	struct tx_desc* tx_next = &tx_list[tail];
	//如果包长度超出了，就直接截住
	if (length > sizeof(struct packets))
		length = sizeof(struct packets); 
	//如果DD位被设置了，就说明这个传输描述符安全回收，可以传输下一个包，也就是队列还没有满
	if ((tx_next->status & E1000_TXD_STAT_DD) == E1000_TXD_STAT_DD) {
		//没有满，把包复制到缓冲区里面去
		memmove(KADDR(tx_next->addr), addr, length);
		tx_next->status &= !E1000_TXD_STAT_DD; //表示现在该描述符还没被处理
		tx_next->length = (uint16_t)length;
		//更新TDT，注意是循环队列
		pci_e1000[E1000_TDT >> 2] = (tail + 1) % TX_MAX;  
		cprintf("my message:%s, %d, %02x\n", tx_buf[tail].buffer, tx_list[tail].length, tx_list[tail].status);
		return 0;
	}
	//DD位没有被设置，传输队列满了
	return -1;
}

int
e1000_receive(void* addr) {

	//int head = pci_e1000[E1000_RDH >> 2];
	int tail = (pci_e1000[E1000_RDT >> 2]+1)%RX_MAX;
	// 有效描述符指的是可供E1000存放接收到数据的描述符或者是说数据已经被读取过了
	struct rx_desc* rx_valid = &rx_list[tail];
	if ((rx_valid->status & E1000_TXD_STAT_DD) == E1000_TXD_STAT_DD) {
		int len = rx_valid->length;
		memcpy(addr, rx_buf[tail].buffer, len);
		rx_valid->status &= ~E1000_TXD_STAT_DD;
		pci_e1000[E1000_RDT >> 2] = tail;
		return len;

	}
	return -1;
}

void 
e1000_transmit_init(){

	memset(tx_list, 0, sizeof(struct tx_desc) * TX_MAX);
	memset(tx_buf, 0, sizeof(struct packets) * TX_MAX);
	//每一个传输描述符都对应着一个packet
	for (int i = 0; i < TX_MAX; i++) {
		tx_list[i].addr = PADDR(tx_buf[i].buffer); //不太懂为什么可以用PADDR
		//24=4(16进制相对于2进制)*6
		tx_list[i].cmd = (E1000_TXD_CMD_RS >> 24) | (E1000_TXD_CMD_EOP >> 24);
		//因为RSV除了82544GC/EI之外，所有以太网控制器都应该保留这个位，并将其编程为0b。
		//而LC，EC在全双工没有意义，
		tx_list[i].status = E1000_TXD_STAT_DD;
	}
	//(TDBAL/TDBAH)指向传输描述符队列的base和high
	//将pci_e1000视为数组的话，他存放的元素应该是32位的，
	//以太网控制器中的寄存器都是32位的，
	pci_e1000[E1000_TDBAL >> 2] = PADDR(tx_list);
	pci_e1000[E1000_TDBAH >> 2] = 0;
	pci_e1000[E1000_TDLEN >> 2] = TX_MAX * sizeof(struct tx_desc);
	pci_e1000[E1000_TDH >> 2] = 0;
	pci_e1000[E1000_TDT >> 2] = 0;
	pci_e1000[E1000_TCTL >> 2] |= (E1000_TCTL_EN | E1000_TCTL_PSP |
		(E1000_TCTL_CT & (0x10 << 4)) |
		(E1000_TCTL_COLD & (0x40 << 12)));
	pci_e1000[E1000_TIPG >> 2] |= (10) | (4 << 10) | (6 << 20);
}

void
e1000_receive_init()
{
	for(int i=0; i<RX_MAX; i++){
		memset(&rx_list[i], 0, sizeof(struct rx_desc));
		memset(&rx_buf[i], 0, sizeof(struct packets));
		rx_list[i].addr = PADDR(rx_buf[i].buffer); 
		//rx_list[i].status &= ~E1000_TXD_STAT_DD; 
	}
	pci_e1000[E1000_MTA>>2] = 0;
	pci_e1000[E1000_RDBAL>>2] = PADDR(rx_list);
	pci_e1000[E1000_RDBAH>>2] = 0;
	pci_e1000[E1000_RDLEN>>2] = RX_MAX*sizeof(struct rx_desc);
	pci_e1000[E1000_RDH>>2] = 0;
	pci_e1000[E1000_RDT>>2] = RX_MAX -1;
	pci_e1000[E1000_RCTL>>2] = (E1000_RCTL_EN | E1000_RCTL_BAM |
				    E1000_RCTL_LBM_NO | E1000_RCTL_SZ_2048 |
				     E1000_RCTL_SECRC);
	pci_e1000[E1000_RA>>2] = 0x52 | (0x54<<8) | (0x00<<16) | (0x12<<24);
	pci_e1000[(E1000_RA>>2) + 1] = (0x34) | (0x56<<8) | E1000_RAH_AV;
}
