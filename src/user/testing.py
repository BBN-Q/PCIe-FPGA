import numpy as np
import resource
def aligned(size, dtype, alignment=resource.getpagesize()):
	#Allocate a little more
	extra = alignment/np.dtype(dtype).itemsize
	buf = np.empty(size + extra, dtype=dtype)

	if (buf.ctypes.data % alignment) == 0:
		return buf
	else:
		offset = (-buf.ctypes.data % alignment) / buf.itemsize
		return buf[offset:offset+size].view(dtype=dtype)

def fill_buffer(buf):
	bytesRead = lib.pulse_stream(buf.size, buf.ctypes.data_as(ctypes.POINTER(ctypes.c_uint16)))
	buf.byteswap(True)

lib.write_register(0, 1)
fill_buffer(buf)
lib.write_register(0, 0)


