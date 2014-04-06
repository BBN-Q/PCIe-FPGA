// Copyright 2014 Raytheon BBN Technologies
// Original Author: Colm Ryan (cryan@bbn.com)

#include <iostream>

#include <random>
#include <chrono>
#include <vector>
#include <thread>

//Use Linux file functions for the device
#include <fcntl.h>
#include <unistd.h> // opening flags 

using std::cout;
using std::endl;
using std::vector;

struct IOCmd_t {
	uint32_t cmd; //command word
	uint8_t barNum; //which bar we are read/writing to
	uint32_t devAddr; // address relative to BAR that we are read/writing to
	void * userAddr; // virtual address in user space to read/write from
};

int main(int argc, char const *argv[])
{
	cout << "Starting to read pulse counts" << endl;

	//Open the device
	//TODO: sort out what flags are necessary
	int FID = open("/dev/piecomm1", O_RDWR | O_NONBLOCK);
	if (FID < 0){
		cout << "Could not open device handle!" << endl;
		return -1;
	}

	double countTime = 1;

	//Convert counting time into 10ns counts
	uint32_t countMax = static_cast<uint32_t>(countTime*1e8);

	//Write the count register
	IOCmd_t iocmd = {0,0,0,0};
	uint32_t regValue;
	regValue = countMax;
	iocmd.userAddr = &regValue;
	iocmd.devAddr = 0x8;
	write(FID, &iocmd, sizeof(regValue));

	//Write the LED register
	regValue = 0x00200000u;
	iocmd.devAddr = 0x4;
	write(FID, &iocmd, sizeof(regValue));

	//Now wait for countTime
	std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int>(countTime*1.1e6)));
	
	//Now read out the results 
	for (size_t ct = 0; ct < 16; ++ct)
	{
		iocmd.devAddr = 0x40 + 4*ct;
		read(FID, &iocmd, sizeof(regValue));
		cout << "0x" << std::hex << regValue << " ";
	}

	cout << endl;		

	close(FID);

	return 0;
}