// Copyright 2014 Raytheon BBN Technologies
// Original Author: Colm Ryan (cryan@bbn.com)

#include <iostream>

#include <random>
#include <chrono>
#include <vector>

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


static const int MEMSIZE = 65536;


bool test_small_writes(int FID, size_t numTries){

	bool success = true;
	uint8_t testWrite8, testRead8;
	uint32_t testWrite32, testRead32;

	IOCmd_t iocmd = {0,0,0,0};

 	std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> byteDis;
    std::uniform_int_distribution<uint32_t> wordDis;
    std::uniform_int_distribution<uint32_t> addrDis(0, MEMSIZE-1);
 	
 	for (size_t ct = 0; ct < numTries; ct++)
	{
		iocmd.devAddr = addrDis(gen);
		testWrite8 =  byteDis(gen);
		iocmd.userAddr = &testWrite8;
		write(FID, &iocmd, sizeof(testWrite8));
		iocmd.userAddr = &testRead8;
		read(FID, &iocmd, sizeof(testRead8));
		if (testWrite8 != testRead8){
			cout << "Failed write/read test at address " << iocmd.devAddr << ": wrote " <<  static_cast<int>(testWrite8) << "; read " << static_cast<int>(testRead8) << endl;
			success = false;
		}
	}

 	for (size_t ct = 0; ct < numTries; ct++)
	{
		iocmd.devAddr = addrDis(gen);
		testWrite32 =  wordDis(gen);
		iocmd.userAddr = &testWrite32;
		write(FID, &iocmd, sizeof(testWrite32));
		iocmd.userAddr = &testRead32;
		read(FID, &iocmd, sizeof(testRead32));
		if (testWrite32 != testRead32){
			cout << "Failed write/read test at address " << iocmd.devAddr << ": wrote " <<  static_cast<int>(testWrite32) << "; read " << static_cast<int>(testRead32) << endl;
			success = false;
		}
	}
	return success;
}


bool test_long_write(int FID){

	bool success = true;
	IOCmd_t iocmd = {0,0,0,0};

 	std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> byteDis;
    std::uniform_int_distribution<uint32_t> addrDis(0, MEMSIZE/2-1); 	

    //Create a random vector of half the length of the block RAM
    vector<uint8_t> testVec;
    for (size_t ct=0; ct<32768; ct++){
    	testVec.push_back(byteDis(gen));
    }

    iocmd.devAddr = addrDis(gen);
    iocmd.userAddr = testVec.data();

    auto start = std::chrono::steady_clock::now();

    write(FID, &iocmd, testVec.size());

    auto end = std::chrono::steady_clock::now();

    cout << "Writing 32kB took " << std::chrono::duration_cast<std::chrono::microseconds>(end-start).count() << " us" << endl;

    vector<uint8_t> testVec2;
    testVec2.resize(32768);
    iocmd.userAddr = testVec2.data();

    start = std::chrono::steady_clock::now();
    read(FID, &iocmd, testVec2.size());
    end = std::chrono::steady_clock::now();

    cout << "Reading 32kB took " << std::chrono::duration_cast<std::chrono::microseconds>(end-start).count() << " us" << endl;

	for (size_t ct=0; ct<32768; ct++){
		if (testVec[ct] != testVec2[ct]){
			cout << "Read/write failed at " << ct << endl;
			success = false;
		}
	}
	return success;
}



int main(int argc, char const *argv[])
{
	cout << "Starting test of PIECOMM kernel module." << endl;


	//Open the device
	//TODO: sort out what flags are necessary
	int FID = open("/dev/piecomm1", O_RDWR | O_NONBLOCK);
	if (FID < 0){
		cout << "Could not open device handle!" << endl;
		return -1;
	}

	bool testResult = test_small_writes(FID, 100);
	if (testResult){
		cout << "Successfully tested byte and word write/reads." << endl;
	}

	testResult = test_long_write(FID);
	if (testResult){
		cout << "Successfully tested long write/read." << endl;
	}


	close(FID);

	return 0;
}