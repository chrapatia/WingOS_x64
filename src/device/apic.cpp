#include <arch/arch.h>
#include <arch/mem/virtual.h>
#include <arch/pic.h>
#include <com.h>
#include <device/acpi.h>
#include <device/apic.h>
#include <device/local_data.h>
apic main_apic = apic();

enum ioapic_register
{
    version_reg = 0x1
};
enum ioapic_flags
{
    active_high_low = 2,
    edge_level = 8
};

apic::apic()
{
    loaded = false;
}

void apic::io_write(uint64_t base, uint32_t reg, uint32_t data)
{

    base = (uint32_t)get_mem_addr(base);
    POKE(base) = reg;
    POKE(base + 16) = data;
}
uint32_t apic::io_read(uint64_t base, uint32_t reg)
{
    base = (uint32_t)get_mem_addr(base);
    POKE(base) = reg;
    return POKE(base + 16);
}
void apic::enable()
{
    write(sivr, read(sivr) | 0x1FF);
}
void apic::EOI()
{
    write(eoi, 0);
}

bool apic::isloaded()
{
    return loaded;
}
void apic::init()
{
    printf(" ========== APIC ========== \n");
    printf("loading apic \n");
    apic_addr = (void *)((uint64_t)madt::the()->lapic_base);
    printf("apic address %x \n", apic_addr);
    if (apic_addr == nullptr)
    {
        printf("[error] can't find apic (sad) \n");
        while (true)
        {
            asm("hlt");
        }
        return;
    }

    x86_wrmsr(0x1B, (x86_rdmsr(0x1B) | 0x800) & ~(LAPIC_ENABLE));
    enable();

    outb(PIC1_DATA, 0xff); // mask all for apic
    pic_wait();
    outb(PIC2_DATA, 0xff);
    printf("loading apic : OK \n");
    printf("loading io apic \n");

    table = madt::the()->get_madt_ioAPIC();
    for (int i = 0; table[i] != 0; i++)
    {
        printf("getting io apic %x \n", i);
        uint64_t addr = (table[i]->ioapic_addr);
        uint32_t raw_table = (io_read(addr, version_reg));
        io_apic_version_table *tables = (io_apic_version_table *)&raw_table;
        io_version_data = *tables;
        printf("configuring io apic %x | version %x | max entry count %x \n", i, tables->version, tables->maximum_redirection);
        printf("gsi start %x | gsi end %x \n", table[i]->gsib, table[i]->gsib + tables->maximum_redirection);
    }
    iso_table = madt::the()->get_madt_ISO();
    for (int i = 0; iso_table[i] != 0; i++)
    {
        printf("getting iso %x \n", i);
        printf("source %x | target %x \n", iso_table[i]->irq, iso_table[i]->interrupt);
        //printf("source %x | target %x \n", iso_table[i]->irq, iso_table[i]->interrupt);
        if (iso_table[i]->misc_flags & 0x4)
        {
            printf("iso is active high \n");
        }
        else
        {
            printf("iso is active low \n");
        }
        if (iso_table[i]->misc_flags & 0x100)
        {
            printf("iso edge triggered \n");
        }
        else
        {
            printf("iso level triggered \n");
        }
    }
    printf("current processor id %x \n", get_current_processor_id());
    lapic_eoi_ptr = (uint32_t *)((uint64_t)apic_addr + 0xb0);
    loaded = true;
}

uint32_t apic::read(uint32_t regs)
{
    return *((volatile uint32_t *)((uint64_t)apic_addr + regs));
}

void apic::write(uint32_t regs, uint32_t val)
{

    *((volatile uint32_t *)(((uint64_t)apic_addr) + regs)) = val;
}
uint32_t apic::IO_get_max_redirect(uint32_t apic_id)
{

    uint64_t addr = (table[apic_id]->ioapic_addr);
    uint32_t raw_table = (io_read(addr, version_reg));
    io_apic_version_table *tables = (io_apic_version_table *)&raw_table;
    return tables->maximum_redirection;
}
void apic::set_apic_addr(uint32_t new_address)
{
    // not used
}
apic *apic::the()
{
    return &main_apic;
}
uint32_t apic::get_current_processor_id()
{
    return (read(lapic_id) >> 24);
}
void apic::preinit_processor(uint32_t processorid)
{

    write(icr2, (processorid << 24));
    write(icr1, 0x500);
}

void apic::init_processor(uint32_t processorid, uint64_t entry)
{

    write(icr2, (processorid << 24));
    write(icr1, 0x600 | ((uint32_t)entry / 4096));
}
void apic::set_raw_redirect(uint8_t vector, uint32_t target_gsi, uint16_t flags, int cpu, int status)
{
    // get io apic from target

    uint64_t end = vector;

    int64_t io_apic_target = -1;
    for (uint64_t i = 0; table[i] != 0; i++)
    {
        if (table[i]->gsib <= target_gsi)
        {
            if (table[i]->gsib + IO_get_max_redirect(i) > target_gsi)
            {
                io_apic_target = i;
                break;
            }
        }
    }
    if (io_apic_target == -1)
    {
        printf("error while trying to setup raw redirect for io apic :( no iso table found ");
        return;
    }

    if (flags & active_high_low)
    {
        end |= (1 << 13);
    }
    if (flags & edge_level)
    {
        end |= (1 << 15);
    }
    if (!status)
    {
        end |= (1 << 16);
    }
    printf("### current cpu lapic %x \n", (uint64_t)get_current_data(cpu)->lapic_id);
    end |= (((uint64_t)get_current_data(cpu)->lapic_id) << 56);
    uint32_t io_reg = (target_gsi - table[io_apic_target]->gsib) * 2 + 16;
    io_write(table[io_apic_target]->ioapic_addr, io_reg, (uint32_t)end);
    io_write(table[io_apic_target]->ioapic_addr, io_reg + 1, (uint32_t)(end >> 32));
}

void apic::set_redirect_irq(int cpu, uint8_t irq, int status)
{

    printf("setting redirect irq cpu %x irq %x status %x \n", cpu, irq, status);
    for (uint64_t i = 0; iso_table[i] != 0; i++)
    {
        if (iso_table[i]->irq == irq)
        {
            printf("found iso matching %x, mapping to source : %x, gsi %x \n", i, iso_table[i]->irq + 0x20, iso_table[i]->interrupt);

            set_raw_redirect(iso_table[i]->irq + 0x20, iso_table[i]->interrupt, iso_table[i]->misc_flags, cpu, status);
            return;
        }
    }
    set_raw_redirect(irq + 0x20, irq, 0, cpu, status);
}