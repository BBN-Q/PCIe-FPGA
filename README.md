PCIe-FPGA
=========

A linux kernel driver for PCIe - FPGA communication.  

Quick Start
============

FPGA
------
The driver is currently written for a Arria II GX dev. board using a hard IP PCI core and a QSys memory map.  A block RAM is used as a ring buffer to copy data to the PC.  

1. Load the sof bitfile into the FPGA either via the USB-Blaster or the web interface that boots from the reference design in flash. If the bit file loads correctly the debugging LED's will flash out a counting pattern to let you know it's alive. 
2. Perform a software-reboot of the computer so the PCI bus picks up the new configuration.  Keep an eye on the LED's to make sure the board didn't lose power. 


Driver Software
---------------

1. Clone/download the repository into some convenient location.  
2. Open a shell prompt and move into the repo. directory: ```cd /path/to/repo```
3. Move into the driver folder and compile the driver: ```cd src/driver; make```
4. Run the load_driver script as root: ```sudo sh ./load_driver.sh```
5. Move back into the test directory.  Compile the test program with: ```g++ -std=c++11 test.cpp -o test.out``` and then run the executable for some simple read/write tests.



Dependencies
=============

FPGA
----------
The bitfile was compiled using Quartus 13.1.  There is a qsys memory map using the [Altera modular DMA engine](http://www.alterawiki.com/wiki/Modular_SGDMA). 

C++ Code
-----------
A recent gcc compiler with C++11 support.  gcc 4.7.3 or later should work.

Python
---------
All of these are available with a simple ```pip install x```
* numpy
* matplotlib
* enaml
* atom
* brewer2mpl

 
