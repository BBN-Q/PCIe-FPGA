# Copyright 2014 Raytheon BBN Technologies
# Original Author: Colm Ryan (cryan@bbn.com)

from atom.api import Atom, Float, Bool

import enaml
from enaml.qt.qt_application import QtApplication

import matplotlib.pyplot as plt
import brewer2mpl

import numpy as np

import ctypes

from itertools import product
from random import choice
from time import sleep

class PulseCounter(Atom):
	repRate = Float(0.0)
	windowed = Bool(False)
	window = Float(0.0)
	delay = Float(0.0)

#Decoder dictionary

#Single counts dictionary  to pixels where we get one row or colum is all pop counts of one
singleCounts = { ((1 << r) + (1 << c)):ct for ct, (r,c) in enumerate( product(range(8,16), range(8)) )}

def fake_counts(n):
	""" Fake data acquisition with a 100ms delay. """
	sleep(0.1)
	return np.array([choice(singleCounts.keys()) for _ in range(n)], dtype=np.uint16)

def count_pixels(counter):
	#Get some counts
	#TODO: set window parameters
	#TODO: set enable
	
	#Get the raw counts
	counts = fake_counts(10000)

	#Reduce to pixels
	pixelizer = np.vectorize(lambda x: singleCounts[x])
	pixelCounts = pixelizer(counts)
	pixelCounts.resize((8,8))

	return pixelCounts 


reds = brewer2mpl.get_map('Reds', 'Sequential', 9).mpl_colormap

def plot_counts(pixelCounts):
	fig, ax = plt.subplots(1)
	img = ax.pcolorfast(pixelCounts, cmap=reds)
	plt.colorbar(img)
	return fig

if __name__ == '__main__':
	
	counter = PulseCounter()

	fig, ax = plt.subplots(1)

	with enaml.imports():
		from PulseCounterView import PulseCounterMainWin

	app = QtApplication()
	# Create a view and show it.
	view = PulseCounterMainWin(counter=counter, fig=fig)
	view.show()

	app.start()