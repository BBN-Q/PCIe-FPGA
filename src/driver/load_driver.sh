#!/bin/bash
#Bash script to load BBN FPGA PCIe driver.  Must be run with sudo rights. 

# Copyright 2014 Raytheon BBN Technologies
# Original Author: Colm Ryan (cryan@bbn.com)


# Load the BBN FPGA PCIe driver
if [ `lsmod | grep -o BBN_FPGA` ]; then
	echo "BBN FPGA driver has already been loaded. Doing nothing."
	exit
fi
insmod BBN_FPGA.ko

#Find what major device number was assigned from /proc/devices
majorNum=$( awk '{ if ($2 ~ /piecomm1/) print $1}' /proc/devices )

if [ -z "$majorNum" ]; then
	echo "Unable to find the BBN_FPGA device!"
	echo "Did the driver correctly load?"
else
	#Remove any stale device file
	if [ -e "/dev/piecomm1" ]; then
		rm -r /dev/piecomm1
	fi

	#Create a new one with full read/write permissions for everyone
	sudo mknod -m 666 /dev/piecomm1 c $majorNum 0
fi


