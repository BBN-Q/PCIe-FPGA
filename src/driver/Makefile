obj-m := BBN_FPGA_DRV.o

BBN_FPGA_DRV-y := DMA.o BBN_FPGA.o

KVERSION = $(shell uname -r)

all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
