#ifndef DMA_H
#define DMA_H

#include <stdint.h>

enum DMA_TI{
    INTEN = 0,
    WAIT_RESP = 3,
    DEST_INC = 4,
    DEST_WIDTH = 5,
    DEST_DREQ = 6,
    DEST_IGNORE = 7,
    SRC_INC = 8,
    SRC_WIDTH = 9,
    SRC_DREQ = 10,
    SRC_IGNORE = 11,
    BURST_LENGTH = 12,
    PERMAP = 16,
    WAITS = 21,
    NO_WIDE_BURSTS = 26
};

#define DMA_CH_0   (*((volatile DMA_Channel *)(0x3F007000)))
#define DMA_ENABLE     (*((volatile uint32_t *)(0x3F007FF0)))

typedef struct{
    uint32_t control_status;
    uint32_t control_block_addr;
    uint32_t transfer_information;
    uint32_t source_addr;
    uint32_t dest_addr;
    uint32_t tranfer_length;
    uint32_t stride;
    uint32_t next_control_block;
    uint32_t debug;
} DMA_Channel;

typedef struct{
    uint32_t transfer_information;
    uint32_t source_addr;
    uint32_t dest_addr;
    uint32_t transfer_length;
    uint32_t stride;
    uint32_t next_control_block;
    uint32_t dummy0;
    uint32_t dummy1;
} DMA_ControlBlock __attribute__ ((aligned(32)));

#endif