// Copyright 2014 Raytheon BBN Technologies
// Original Author: Colm Ryan (cryan@bbn.com)

#include "BBN_FPGA.h"


MODULE_LICENSE("MIT");
MODULE_AUTHOR("Colm Ryan (cryan@bbn.com)");
MODULE_DESCRIPTION("Driver for PCI Altera FPGA boards with a PCI Qsys interface.");

/* Forward Static PCI driver functions for kernel module */
static int probe(struct pci_dev *dev, const struct pci_device_id *id);
static void remove(struct pci_dev *dev);

// static int  init_chrdev (struct aclpci_dev *aclpci);


//Fill in kernel structures with a list of ids this driver can handle
static struct pci_device_id idTable[] = {
	{ PCI_DEVICE(VENDOR_ID, DEVICE_ID) },
	{ 0, },
};
MODULE_DEVICE_TABLE(pci, idTable);

static struct pci_driver fpgaDriver = {
	.name = DRIVER_NAME,
	.id_table = idTable,
	.probe = probe,
	.remove = remove,
};

struct file_operations fileOps = {
	.owner =    THIS_MODULE,
	.read =     fpga_read,
	.write =    fpga_write,
	.open =     fpga_open,
	.release =  fpga_close,
};


/*I/0 - should move to separate file at some point */
struct IOCmd_t {
	uint32_t cmd; //command word
	uint8_t barNum; //which bar we are read/writing to
	uint32_t devAddr; // address relative to BAR that we are read/writing to
	void * userAddr; // virtual address in user space to read/write from
};

ssize_t rw_dispatcher(struct file *filePtr, char __user *buf, size_t count, bool rwFlag){

	//Read the command from the buffer
	struct IOCmd_t iocmd; 
	void * startAddr;

	size_t bytesDone = 0;
	size_t bytesToTransfer = 0;

	struct DevInfo_t * devInfo = (struct DevInfo_t *) filePtr->private_data;

	printk(KERN_INFO "[BBN FPGA] rw_dispatcher: Entering function.\n");

	if (down_interruptible(&devInfo->sem)) {
		printk(KERN_WARNING "[BBN FPGA] rw_dispatcher: Unable to get semaphore!\n");
		return -1;
	}

	copy_from_user(&iocmd, (void __user *) buf, sizeof(iocmd));

	//Map the device address to the iomaped memory
	startAddr = (void*) (devInfo->bar[iocmd.barNum] + iocmd.devAddr) ;

	printk(KERN_INFO "[BBN FPGA] rw_dispatcher: Reading/writing %u bytes from user address 0x%p to device address %u.\n", (unsigned int) count, iocmd.userAddr, iocmd.devAddr);
	while (count > 0){
		bytesToTransfer = (count > BUFFER_SIZE) ? BUFFER_SIZE : count;


		if (rwFlag) {
			//First read from device into kernel memory 
			memcpy_fromio(devInfo->buffer, startAddr + bytesDone, bytesToTransfer);
			//Then into user space
			copy_to_user(iocmd.userAddr + bytesDone, devInfo->buffer, bytesToTransfer);
		}
		else{
			//First copy from user to buffer
			copy_from_user(devInfo->buffer, iocmd.userAddr + bytesDone, bytesToTransfer);
			//Then into the device
			memcpy_toio(startAddr + bytesDone, devInfo->buffer, bytesToTransfer);
		}
		bytesDone += bytesToTransfer;
		count -= bytesToTransfer;
	}
	up(&devInfo->sem);
	return bytesDone;
}

int fpga_open(struct inode *inode, struct file *filePtr) {
	//Get a handle to our devInfo and store it in the file handle
	struct DevInfo_t * devInfo = 0;

	printk(KERN_INFO "[BBN FPGA] fpga_open: Entering function.\n");

	devInfo = container_of(inode->i_cdev, struct DevInfo_t, cdev);

	if (down_interruptible(&devInfo->sem)) {
		printk(KERN_WARNING "[BBN FPGA] fpga_open: Unable to get semaphore!\n");
		return -1;
	}

	filePtr->private_data = devInfo;

	//Record the PID of who opened the file
	//TODO: sort out where this is used
	devInfo->userPID = current->pid;

	//Return semaphore
	up(&devInfo->sem);

	if (down_interruptible(&devInfo->sem)) {
		printk(KERN_WARNING "[BBN FPGA] fpga_open: Unable to get semaphore!\n");
		return -1;
	}

	up(&devInfo->sem);

	printk(KERN_INFO "[BBN FPGA] fpga_open: Leaving function.\n");

	return 0;
}
int fpga_close(struct inode *inode, struct file *filePtr){

	struct DevInfo_t * devInfo = (struct DevInfo_t *)filePtr->private_data;

	if (down_interruptible(&devInfo->sem)) {
		printk(KERN_WARNING "[BBN FPGA] fpga_close: Unable to get semaphore!\n");
		return -1;
	}

	//TODO: some checking of who is closing.
	up(&devInfo->sem);

	return 0;
}

//Pass-through to main dispatcher
ssize_t fpga_read(struct file *filePtr, char __user *buf, size_t count, loff_t *pos){

	return rw_dispatcher(filePtr, buf, count, true);
}
ssize_t fpga_write(struct file *filePtr, const char __user *buf, size_t count, loff_t *pos){

		return rw_dispatcher(filePtr, (char __user *) buf, count, false);
}

static int setup_chrdev(struct DevInfo_t *devInfo){
	/*
	Setup the /dev/deviceName to allow user programs to read/write to the driver.
	*/

	int devMinor = 0;
	int devMajor = 0; 
	int devNum = -1;

	int result = alloc_chrdev_region(&devInfo->cdevNum, devMinor, 1 /* one device*/, BOARD_NAME);
	if (result < 0) {
		printk(KERN_WARNING "Can't get major ID\n");
		return -1;
	}
	devMajor = MAJOR(devInfo->cdevNum);
	devNum = MKDEV(devMajor, devMinor);
	
	//Initialize and fill out the char device structure
	cdev_init(&devInfo->cdev, &fileOps);
	devInfo->cdev.owner = THIS_MODULE;
	devInfo->cdev.ops = &fileOps;
	result = cdev_add(&devInfo->cdev, devNum, 1 /* one device */);
	if (result) {
		printk(KERN_NOTICE "Error %d adding char device for BBN FPGA driver with major/minor %d / %d", result, devMajor, devMinor);
		return -1;
	}

	return 0;
}

static int map_bars(struct DevInfo_t *devInfo){
	/*
	Map the device memory regions into kernel virtual address space.
	Report their sizes in devInfo.barLengths
	*/
	int ct = 0;
	unsigned long barStart, barEnd, barLength;
	for (ct = 0; ct < NUM_BARS; ct++){
		printk(KERN_INFO "[BBN FPGA] Trying to map BAR #%d of %d.\n", ct, NUM_BARS);
		barStart = pci_resource_start(devInfo->pciDev, ct);
		barEnd = pci_resource_end(devInfo->pciDev, ct);
		barLength = barEnd - barStart + 1;

		devInfo->barLengths[ct] = barLength;

		//Check for empty BAR
		if (!barStart || !barEnd) {
			devInfo->barLengths[ct] = 0;
			printk(KERN_INFO "[BBN FPGA] Empty BAR #%d.\n", ct);
			continue;
		}

		//Check for messed up BAR
		if (barLength < 1) {
			printk(KERN_WARNING "[BBN FPGA] BAR #%d length is less than 1 byte.\n", ct);
			continue;
		}

		// If we have a valid bar region then map the device memory or
		// IO region into kernel virtual address space  
		devInfo->bar[ct] = pci_iomap(devInfo->pciDev, ct, barLength);

		if (!devInfo->bar[ct]) {
			printk(KERN_WARNING "[BBN FPGA] Could not map BAR #%d.\n", ct);
			return -1;
		}

		printk(KERN_INFO "[BBN FPGA] BAR%d mapped at 0x%p with length %lu.\n", ct, devInfo->bar[ct], barLength);
	}
	return 0;
}  

static int unmap_bars(struct DevInfo_t * devInfo){
	/* Release the mapped BAR memory */
	int ct = 0;
	for (ct = 0; ct < NUM_BARS; ct++) {
		if (devInfo->bar[ct]) {
			pci_iounmap(devInfo->pciDev, devInfo->bar[ct]);
			devInfo->bar[ct] = NULL;
		}
	}
	return 0;
}

static int probe(struct pci_dev *dev, const struct pci_device_id *id) {
		/*
		From : http://www.makelinux.net/ldd3/chp-12-sect-1
		This function is called by the PCI core when it has a struct pci_dev that it thinks this driver wants to control.
		A pointer to the struct pci_device_id that the PCI core used to make this decision is also passed to this function. 
		If the PCI driver claims the struct pci_dev that is passed to it, it should initialize the device properly and return 0. 
		If the driver does not want to claim the device, or an error occurs, it should return a negative error value.
		*/

		//Initalize driver info 
		struct DevInfo_t *devInfo = 0;

		printk(KERN_INFO "[BBN FPGA] Entered driver probe function.\n");
		printk(KERN_INFO "[BBN FPGA] vendor = 0x%x, device = 0x%x \n", dev->vendor, dev->device); 

		//Allocate and zero memory for devInfo

		devInfo = kzalloc(sizeof(struct DevInfo_t), GFP_KERNEL);
		if (!devInfo) {
			printk(KERN_WARNING "Couldn't allocate memory for device info!\n");
			return -1;
		}

		//Copy in the pci device info
		devInfo->pciDev = dev;

		//Save the device info itself into the pci driver
		dev_set_drvdata(&dev->dev, (void*) devInfo);

		//Setup the char device
		setup_chrdev(devInfo);    

		//Initialize other fields
		devInfo->userPID = -1;
		devInfo->buffer = kmalloc (BUFFER_SIZE * sizeof(char), GFP_KERNEL);

		//Enable the PCI
		if (pci_enable_device(dev)){
			printk(KERN_WARNING "[BBN FPGA] pci_enable_device failed!\n");
			return -1;
		}

		pci_set_master(dev);
		pci_request_regions(dev, DRIVER_NAME);

		//Memory map the BAR regions into virtual memory space
		map_bars(devInfo);

		//TODO: proper error catching and memory releasing
		sema_init(&devInfo->sem, 1);

		return 0;

}

static void remove(struct pci_dev *dev) {

	struct DevInfo_t *devInfo = 0;
	
	printk(KERN_INFO "[BBN FPGA] Entered BBN FPGA driver remove function.\n");
	
	devInfo = (struct DevInfo_t*) dev_get_drvdata(&dev->dev);
	if (devInfo == 0) {
		printk(KERN_WARNING "[BBN FPGA] remove: devInfo is 0");
		return;
	}

	//Clean up the char device
	cdev_del(&devInfo->cdev);
	unregister_chrdev_region(devInfo->cdevNum, 1);

	//Release memory
	unmap_bars(devInfo);

	//TODO: does order matter here?
	pci_release_regions(dev);
	pci_disable_device(dev);

	kfree(devInfo->buffer);
	kfree(devInfo);

}

static int fpga_init(void){
	printk(KERN_INFO "[BBN FPGA] Loading BBN FPGA driver!\n");
	return pci_register_driver(&fpgaDriver);
}

static void fpga_exit(void){
	printk(KERN_INFO "[BBN FPGA] Exiting BBN FPGA driver!\n");
	pci_unregister_driver(&fpgaDriver);
}

module_init(fpga_init);
module_exit(fpga_exit);