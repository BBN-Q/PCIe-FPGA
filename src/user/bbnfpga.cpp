#include <iostream>

//Use Linux file functions for the device
#include <fcntl.h>
#include <unistd.h> // opening flags 

#include "bbnfpga.h"

using std::cout;
using std::endl;

//Handle to device
static int FID;

//Read/write command structure
struct IOCmd_t {
	uint32_t cmd; //command word
	uint8_t barNum; //which bar we are read/writing to
	uint32_t devAddr; // address relative to BAR that we are read/writing to
	void * userAddr; // virtual address in user space to read/write from
};


void init() {
	//there is no guarantee iostream has been initialized by this point
	//see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=39796
	std::ios_base::Init i; 
	cout << "Initializing libaps library....";
	FID = open("/dev/piecomm1", O_RDWR | O_NONBLOCK);
	cout << " complete." << endl;
}

void cleanup(){
	cout << "Cleaning up....";
	close(FID);
	cout << " complete." << endl;
}

uint32_t read_register(unsigned addr){
	IOCmd_t iocmd = {0,0,0,0};
	uint32_t regValue;
	iocmd.userAddr = &regValue;
	iocmd.devAddr = 4*addr;
	read(FID, &iocmd, sizeof(regValue));
	return regValue;
}

int write_register(unsigned addr, uint32_t val){
	IOCmd_t iocmd = {0,0,0,0};
	iocmd.userAddr = &val;
	iocmd.devAddr = 4*addr;
	return write(FID, &iocmd, sizeof(val));	
}

int pulse_stream(unsigned count, uint16_t * buf){
	/*
	Fill the buffer with the count entries streamed data from the DMA.
	This assumes page aligned memory.
	*/
	IOCmd_t iocmd = {1,0,0,0};
	iocmd.userAddr = buf;
	return read(FID, &iocmd, 2*count);
}
