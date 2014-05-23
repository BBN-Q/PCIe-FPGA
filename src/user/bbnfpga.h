#ifndef BBNFPGA_H
#define BBNFPGA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void init() __attribute__((constructor));
void cleanup() __attribute__((destructor));

uint32_t read_register(unsigned);
int write_register(unsigned, uint32_t);

int pulse_stream(unsigned, uint16_t*);

#ifdef __cplusplus
}
#endif

#endif
