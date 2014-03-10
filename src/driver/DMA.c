// Copyright 2014 Raytheon BBN Technologies
// Original Author: Colm Ryan (cryan@bbn.com)

#include "DMA.h"
#include "BBN_FPGA.h"

int dma_read(char __user * buf, size_t count, struct DevInfo_t * devInfo){

	/*
	Basic steps to DMA
	1. Pin the buffer from user space passed in. 
	2. Get the physical pages associated with that memory.
	3. Create a scatter-gather list of those pages and associate DMA bus addresses. 
	4. For each entry on the list:
		a. write the address into the PCIe core's address translation table. 
			This table translates Avalon memory locations to computer RAM physical addresses. 
		b. Write a descriptor into the SGDMA engine. 
	5. Unpin the memory. 
	*/

	ssize_t firstPage, lastPage, numPages;
	struct page ** pages;
	struct scatterlist * sgList;
	struct scatterlist * sgEntry;
	int result, numDMAs, ct, attRow;
	dma_addr_t pciAddr;
	uint32_t dmaLength, dmaStatus;

	//Step 1. Pin the buffer
	firstPage = ((ssize_t)buf & PAGE_MASK) >> PAGE_SHIFT;
	lastPage = (((ssize_t)buf+count-1) & PAGE_MASK) >> PAGE_SHIFT;
	numPages = lastPage - firstPage + 1;
	printk(KERN_INFO "[BBN FPGA] %u pages for DMA transfer.", (unsigned int)numPages);

	pages = kmalloc(numPages * sizeof(struct page*), GFP_KERNEL);
	sgList = kzalloc(numPages * sizeof(*sgList), GFP_KERNEL);
	sg_init_table(sgList, numPages);

	result = get_user_pages(current, current->mm, (unsigned long)buf & PAGE_MASK, numPages, 1, 0, pages, NULL);

	if (result != numPages){
		printk(KERN_WARNING "[BBN FPGA] Could not pin user pages.");
	}

	//Step 3. Get the scatter gather list
	for(ct=0; ct<numDMAs; ct++){
		sg_set_page(&sgList[ct], pages[ct], PAGE_SIZE, 0);
	}


	numDMAs = pci_map_sg(devInfo->pciDev, sgList, numPages, DMA_FROM_DEVICE);
	printk(KERN_INFO "[BBN FPGA] Need %u DMA transfers.", (unsigned int)numDMAs);

	//Step 4.
	attRow = 0;
	for_each_sg(sgList, sgEntry, numDMAs, ct){
		//Write the address translation table
		pciAddr = sg_dma_address(sgEntry) | 0x1; // +1 to set LSB for 64bit addresses
		iowrite32(pciAddr & 0xFFFFFFFF, devInfo->bar[PCIE_CRA_BAR] + PCIE_CRA_OFFSET + ATT_CRA_OFFSET + 8*attRow);
		iowrite32(pciAddr >> 32, devInfo->bar[PCIE_CRA_BAR] + PCIE_CRA_OFFSET + ATT_CRA_OFFSET + 8*attRow + 4);
		attRow++;

		//Write the DMA descriptor
		//write address (each ATT entry addresses up to 1MB)
		iowrite32(attRow * 0xFFFFF, devInfo->bar[SGDMA_BAR] + SGDMA_DESCRIPTOR_ADDR);
		//don't need a read address
		//write length
		dmaLength = sg_dma_len(sgEntry);
		iowrite32(dmaLength, devInfo->bar[SGDMA_BAR] + SGDMA_DESCRIPTOR_ADDR + 8);
		//write control word
		iowrite32(DESCRIPTOR_CONTROL_GO_MASK | DESCRIPTOR_CONTROL_END_ON_EOP_MASK, devInfo->bar[SGDMA_BAR] + SGDMA_DESCRIPTOR_ADDR + 12);
	}

	//Wait until finished
	dmaStatus = ioread32(devInfo->bar[SGDMA_BAR] + SGDMA_CSR_ADDR);
	while(!(dmaStatus | 0x1)){
		printk(KERN_INFO "[BBN FPGA] Waiting for reads to finish with dmaStatus = 0x%x.", dmaStatus);
		dmaStatus = ioread32(devInfo->bar[SGDMA_BAR] + SGDMA_CSR_ADDR);
	}

	pci_unmap_sg(devInfo->pciDev, sgList, numPages, DMA_FROM_DEVICE);
	kfree(pages);
	kfree(sgList);

	return 0;
}