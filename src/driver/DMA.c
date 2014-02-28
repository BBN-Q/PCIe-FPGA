// Copyright 2014 Raytheon BBN Technologies
// Original Author: Colm Ryan (cryan@bbn.com)

int dma_rw(char __user * buf, size_t count, bool rwFlag ){

	/*
	Basic steps to DMA
	1. Pin the buffer from user space passed in. 
	2. Get the physical pages associated with that memory.
	3. Create a scatter-gather list of those pages and associate DMA bus addresses. 
	4. For each entry on the list: write the address into the PCIe core's address translation table. 
		This table translates Avalon memory locations to computer RAM physical addresses. Write a descriptior
		into the SGDMA engine. 
	5. Unpin the memory. 
	*/

	
	return 0;
}