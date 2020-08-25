#include <int_value.h>
#include <stdint.h>
#include <stivale.h>
#define GDT_DESCRIPTORS 7

#define SLTR_NULL 0x0000
#define SLTR_KERNEL_CODE 0x0008
#define SLTR_KERNEL_DATA 0x0010
#define SLTR_USER_DATA 0x0018
#define SLTR_USER_CODE 0x0020
#define SLTR_TSS 0x0028 /* occupies two GDT descriptors */

#define RPL0 0x0
#define RPL1 0x1
#define RPL2 0x2
#define RPL3 0x3

typedef struct
{
    uint16_t len;
    uint64_t addr;
} __attribute__((__packed__)) gdtr_t;

typedef struct
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t flags;
    uint8_t granularity; /* and high limit */
    uint8_t base_high;
} __attribute__((__packed__)) gdt_descriptor_t;

typedef struct
{
    gdt_descriptor_t low;
    struct
    {
        uint32_t base_xhigh;
        uint32_t reserved;
    } high;
} __attribute__((__packed__)) gdt_xdescriptor_t;

void gdt_init(void);

void setup_gdt(unsigned long i);

typedef struct
{
    uint32_t reserved0 __attribute__((aligned(16)));
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((__packed__)) tss_t;
tss_t *get_tss();
void tss_init(uint64_t i);
void tss_set_rsp0(uint64_t rsp0);
