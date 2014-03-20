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
	int result, numDMAs, ct, attRow, is64bit;
	dma_addr_t pciAddr = 0;
	uint32_t pciAddrOffset = 0;
	uint32_t dmaLength, dmaStatus;


	//Step 1. Pin the buffer
	firstPage = ((ssize_t)buf & PAGE_MASK) >> PAGE_SHIFT;
	lastPage = (((ssize_t)buf+count-1) & PAGE_MASK) >> PAGE_SHIFT;
	numPages = lastPage - firstPage + 1;
	dev_dbg(&devInfo->pciDev->dev, "Need %u pages for DMA transfer.\n", (unsigned int)numPages);

	pages = kmalloc(numPages * sizeof(struct page*), GFP_KERNEL);
	sgList = kzalloc(numPages * sizeof(*sgList), GFP_KERNEL);
	sg_init_table(sgList, numPages);

	down_write(&current->mm->mmap_sem);
	result = get_user_pages(current, current->mm, (unsigned long)buf & PAGE_MASK, numPages, 1, 0, pages, NULL);

	if (result != numPages){
		pr_err("[BBN FPGA] Pinned only %u pages when we expected %u.\n", (unsigned int)result, (unsigned int)numPages);
		goto fail_release;
	}
	else{
		dev_dbg(&devInfo->pciDev->dev, "Pinned %u pages\n", (unsigned int)result);
	}

	//Step 3. Get the scatter gather list
	for(ct=0; ct<numPages; ct++){
		dev_dbg(&devInfo->pciDev->dev, "[BBN_FPGA] Setting sg entry for page %u\n", (unsigned int)ct);
		sg_set_page(&sgList[ct], pages[ct], PAGE_SIZE, 0);
	}

	numDMAs = pci_map_sg(devInfo->pciDev, sgList, numPages, DMA_FROM_DEVICE);
	dev_dbg(&devInfo->pciDev->dev, "[BBN FPGA] Need %u DMA transfers.\n", (unsigned int)numDMAs);

	//Step 4.
	//Report whether we can actually do DMA to a 64bit address space
	if(!pci_dma_supported(devInfo->pciDev, 0xFFFFFFFFFFFFFFFF )){
		dev_dbg(&devInfo->pciDev->dev, "Device supports 64bit DMA");
		is64bit = 1;
	}
	else{
		dev_dbg(&devInfo->pciDev->dev, "Device does not support 64bit DMA");
		is64bit = 0;
	}


	attRow = 0;
	for_each_sg(sgList, sgEntry, numDMAs, ct){
		//Write the address translation table
		pciAddr = sg_dma_address(sgEntry);
     	// +1 to set LSB for 64bit addresses
		dev_dbg(&devInfo->pciDev->dev, "Writing ATT entry for DMA transfer %d using pciAddr = %llx", ct, pciAddr);
		iowrite32(pciAddr & 0xFFF00000, devInfo->bar[PCIE_CRA_BAR] + PCIE_CRA_OFFSET + ATT_CRA_OFFSET + 8*attRow);
		// iowrite32(pciAddr >> 32, devInfo->bar[PCIE_CRA_BAR] + PCIE_CRA_OFFSET + ATT_CRA_OFFSET + 8*attRow + 4);

		//Write the DMA descriptor
		//write address (each ATT entry addresses up to 1MB)
		pciAddrOffset = pciAddr & 0xFFFFF;
		dev_dbg(&devInfo->pciDev->dev, "Writing DMA write address = %x to %x", PCIE_TX_PORT_AVALON_OFFSET + attRow * 0x100000 + pciAddrOffset, SGDMA_DESCRIPTOR_ADDR + 4 );
		iowrite32(PCIE_TX_PORT_AVALON_OFFSET + attRow * 0x100000 + pciAddrOffset, devInfo->bar[SGDMA_BAR] + SGDMA_DESCRIPTOR_ADDR + 4);
		//don't need a read address
		//write length
		dmaLength = sg_dma_len(sgEntry);
		dev_dbg(&devInfo->pciDev->dev, "Writing DMA length = %x to %x", dmaLength, SGDMA_DESCRIPTOR_ADDR + 8 );
		iowrite32(dmaLength, devInfo->bar[SGDMA_BAR] + SGDMA_DESCRIPTOR_ADDR + 8);
		//write control word
		iowrite32(DESCRIPTOR_CONTROL_GO_MASK | DESCRIPTOR_CONTROL_END_ON_EOP_MASK, devInfo->bar[SGDMA_BAR] + SGDMA_DESCRIPTOR_ADDR + 12);
		attRow++;
	}

	//Wait until finished
	dev_dbg(&devInfo->pciDev->dev, "Getting DMA status register");
	dmaStatus = ioread32(devInfo->bar[SGDMA_BAR] + SGDMA_CSR_ADDR);
	dev_dbg(&devInfo->pciDev->dev, "Got DMA status 0x%x", dmaStatus);
	while((dmaStatus & 0x1)){
		dev_dbg(&devInfo->pciDev->dev, "Waiting for reads to finish with dmaStatus = 0x%x.", dmaStatus);
		dmaStatus = ioread32(devInfo->bar[SGDMA_BAR] + SGDMA_CSR_ADDR);
	}


	dev_dbg(&devInfo->pciDev->dev, "Unmapping the SG list");
	pci_unmap_sg(devInfo->pciDev, sgList, numPages, DMA_FROM_DEVICE);

	dev_dbg(&devInfo->pciDev->dev, "Releasing the user pages");
	for(ct=0; ct<numPages; ct++){
		set_page_dirty_lock(pages[ct]);
		put_page(pages[ct]);
	}


	dev_dbg(&devInfo->pciDev->dev, "Releasing memory");
	fail_release:
	kfree(pages);
	kfree(sgList);

	dev_dbg(&devInfo->pciDev->dev, "Returning semaphore");
	up_write(&current->mm->mmap_sem);

	return count;
}