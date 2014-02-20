obj-m := PiecommDriver.o

KVERSION = $(shell uname -r)

all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
	g++ -std=c++11 test.cpp -o test
clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
