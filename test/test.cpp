#include <iostream>


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


	uint8_t test8 = 0xFF;

	IOCmd_t iocmd = {0, 0, 0, &test8};

	write(FID, &iocmd, 1);

	cout << "Wrote: " << test8;

	test8 = 0;

	read(FID, &iocmd, 1);

	cout << "; Read: " << test8 << endl;

	close(FID);

	return 0;
}