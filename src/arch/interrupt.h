#pragma once
#include <arch/arch.h>
#include <int_value.h>
typedef struct {
    uint16_t offset_low16;
    uint16_t cs;
    uint8_t ist;
    uint8_t attributes;
    uint16_t offset_mid16;
    uint32_t offset_high32;
    uint32_t zero;
} __attribute__((packed)) idt_entry_t;
typedef struct {
    uint16_t size; // size of the IDT
    uint64_t offset; // address of the IDT
} __attribute__((packed)) idtr_t;



void init_idt(void);
