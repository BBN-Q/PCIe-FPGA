#include <iostream>

#include <random>

//Use Linux file functions for the device
#include <fcntl.h>
#include <unistd.h> // opening flags 

using std::cout;
using std::endl;


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
	close(FID);

	return 0;
}