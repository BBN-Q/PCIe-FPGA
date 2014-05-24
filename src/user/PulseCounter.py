# Copyright 2014 Raytheon BBN Technologies
# Original Author: Colm Ryan (cryan@bbn.com)

from atom.api import Atom, Float, Bool, Instance

import enaml
from enaml.qt.qt_application import QtApplication

import matplotlib.pyplot as plt
import brewer2mpl

import numpy as np

import ctypes

from itertools import product
from random import choice
from time import sleep

import resource

CONTROL_REG = 0
CLKRATE_REG = 1

ENABLE_BIT = 0


def aligned(size, dtype, alignment=resource.getpagesize()):
	"""
	Helper function to allocate an aligned numpy array.
	"""

	extra = alignment/np.dtype(dtype).itemsize
	buf = np.empty(size + extra, dtype=dtype)

	if (buf.ctypes.data % alignment) == 0:
		return buf
	else:
		offset = (-buf.ctypes.data % alignment) / buf.itemsize
		return buf[offset:offset+size].view(dtype=dtype)

def get_pulses(n, lib):
	"""
	Reads DMA stream into a PAGESIZE aligned buffer and .
	"""
	#For some reason, every second word is zero
	assert n*2 % resource.getpagesize() == 0, "Need to fill an integer number of pages."
	buf = aligned(2*n, np.uint16)
	bytesRead = lib.pulse_stream(buf.size, buf.ctypes.data_as(ctypes.POINTER(ctypes.c_uint16)))
	#Swaps between big endian FPGA and little endian x64
	return buf[::2].byteswap(True)

def enable(lib):
	lib.write_register(CONTROL_REG, 1 << ENABLE_BIT)

def disable(lib):
	lib.write_register(CONTROL_REG, 0)

def run_capture(n, lib):
	enable(lib)
	pulseTrain = get_pulses(n, lib)
	disable(lib)
	return pulseTrain

class PulseCounter(Atom):
	repRate = Float(0.0)
	windowed = Bool(False)
	window = Float(0.0)
	delay = Float(0.0)
	lib = Instance(ctypes.CDLL)

#Decoder dictionary

#Single counts dictionary  to pixels where we get one row or colum is all pop counts of one
singleCounts = { ((1 << r) + (1 << c)):ct for ct, (r,c) in enumerate( product(range(8,16), range(8)) )}

def fake_pulses(n):
	""" Fake data acquisition with a 100ms delay. """
	sleep(0.1)
	return np.array([choice(singleCounts.keys()) for _ in range(n)], dtype=np.uint16)

def count_pixels(counter, fakeIt=False):
	#Get some counts
	#TODO: set window parameters
	#TODO: set enable
	
	#Get the raw pulse train
	if fakeIt:
		pulses = fake_pulses(2**12)
	else:
		pulses = run_capture(2**12, counter.lib)

	#Reduce to pixels
	pixelizer = np.vectorize(lambda x: singleCounts[x])
	pixelCounts = pixelizer(pulses)
	pixelCounts.resize((8,8))

	return pixelCounts 


reds = brewer2mpl.get_map('Reds', 'Sequential', 9).mpl_colormap

def plot_counts(pixelCounts):
	fig, ax = plt.subplots(1)
	img = ax.pcolorfast(pixelCounts, cmap=reds)
	plt.colorbar(img)
	return fig

if __name__ == '__main__':
	
	#Load the library
	lib = ctypes.cdll.LoadLibrary("libbbnfpga.so")
	#Slow down the count rate to 100MHz/200
	lib.write_register(CLKRATE_REG, 200)

	counter = PulseCounter(lib=lib)

	with enaml.imports():
		from PulseCounterView import PulseCounterMainWin

	app = QtApplication()
	# Create a view and show it.
	fig, ax = plt.subplots(1)
	view = PulseCounterMainWin(counter=counter, fig=fig, lib=lib)
	view.show()

	app.start()