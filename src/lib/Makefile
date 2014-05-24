# Makefile for shared library
CC = g++
CPPFLAGS = -fPIC -Wall # C flags

NAME := bbnfpga
MAJOR := 0
MINOR := 1
VERSION := $(MAJOR).$(MINOR)

lib: lib$(NAME).so
 
lib$(NAME).so: lib$(NAME).so.$(VERSION)
	ldconfig -v -n .
	ln -s lib$(NAME).so.$(MAJOR) lib$(NAME).so
 
lib$(NAME).so.$(VERSION): $(NAME).o
	$(CC) -shared -Wl,-soname,lib$(NAME).so.$(MAJOR) $^ -o $@
 
clean:
	$(RM) $(NAME)_test *.o *.so*