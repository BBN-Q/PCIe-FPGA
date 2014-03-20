#include <asm/scatterlist.h>

static const uint32_t ATT_CRA_OFFSET = 0x1000;
static const uint32_t PCIE_CRA_OFFSET = 0;
static const int PCIE_CRA_BAR = 2;
static const uint32_t PCIE_TX_PORT_AVALON_OFFSET = 0;


static const uint32_t SGDMA_CSR_ADDR = 0x4000;
static const uint32_t SGDMA_DESCRIPTOR_ADDR = 0x4020;
static const int SGDMA_BAR = 2;


// masks and offsets for the bits in the descriptor control field
#define DESCRIPTOR_CONTROL_TRANSMIT_CHANNEL_MASK         (0xFF)
#define DESCRIPTOR_CONTROL_TRANSMIT_CHANNEL_OFFSET       (0)
#define DESCRIPTOR_CONTROL_GENERATE_SOP_MASK             (1<<8)
#define DESCRIPTOR_CONTROL_GENERATE_SOP_OFFSET           (8)
#define DESCRIPTOR_CONTROL_GENERATE_EOP_MASK             (1<<9)
#define DESCRIPTOR_CONTROL_GENERATE_EOP_OFFSET           (9)
#define DESCRIPTOR_CONTROL_PARK_READS_MASK               (1<<10)
#define DESCRIPTOR_CONTROL_PARK_READS_OFFSET             (10)
#define DESCRIPTOR_CONTROL_PARK_WRITES_MASK              (1<<11)
#define DESCRIPTOR_CONTROL_PARK_WRITES_OFFSET            (11)
#define DESCRIPTOR_CONTROL_END_ON_EOP_MASK               (1<<12)
#define DESCRIPTOR_CONTROL_END_ON_EOP_OFFSET             (12)
#define DESCRIPTOR_CONTROL_TRANSFER_COMPLETE_IRQ_MASK    (1<<14)
#define DESCRIPTOR_CONTROL_TRANSFER_COMPLETE_IRQ_OFFSET  (14)
#define DESCRIPTOR_CONTROL_EARLY_TERMINATION_IRQ_MASK    (1<<15)
#define DESCRIPTOR_CONTROL_EARLY_TERMINATION_IRQ_OFFSET  (15)
#define DESCRIPTOR_CONTROL_ERROR_IRQ_MASK                (0xFF<<16)  // the read master will use this as the transmit error, the dispatcher will use this to generate an interrupt if any of the error bits are asserted by the write master
#define DESCRIPTOR_CONTROL_ERROR_IRQ_OFFSET              (16)
#define DESCRIPTOR_CONTROL_EARLY_DONE_ENABLE_MASK        (1<<24)
#define DESCRIPTOR_CONTROL_EARLY_DONE_ENABLE_OFFSET      (24)
#define DESCRIPTOR_CONTROL_GO_MASK                       (1<<31)  // at a minimum you always have to write '1' to this bit as it commits the descriptor to the dispatcher
#define DESCRIPTOR_CONTROL_GO_OFFSET                     (31)
