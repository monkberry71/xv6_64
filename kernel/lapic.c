#include <stdint.h>
#include "x86_64.h"
#include "vm.h"
#include "memlayout.h"
#include "mmu.h"
#include "trap.h"

#define IA32_APIC_BASE_MSR 0x1B
#define IA32_APIC_BASE_MSR_ENABLE 0x800

// Local APIC registers, divided by 4 for use as uint[] indices.
#define ID      (0x0020/4)   // ID
#define VER     (0x0030/4)   // Version
#define TPR     (0x0080/4)   // Task Priority
#define EOI     (0x00B0/4)   // EOI
#define SVR     (0x00F0/4)   // Spurious Interrupt Vector
  #define ENABLE     0x00000100   // Unit Enable
#define ESR     (0x0280/4)   // Error Status
#define ICRLO   (0x0300/4)   // Interrupt Command
  #define INIT       0x00000500   // INIT/RESET
  #define STARTUP    0x00000600   // Startup IPI
  #define DELIVS     0x00001000   // Delivery status
  #define ASSERT     0x00004000   // Assert interrupt (vs deassert)
  #define DEASSERT   0x00000000
  #define LEVEL      0x00008000   // Level triggered
  #define BCAST      0x00080000   // Send to all APICs, including self.
  #define BUSY       0x00001000
  #define FIXED      0x00000000
#define ICRHI   (0x0310/4)   // Interrupt Command [63:32]
#define TIMER   (0x0320/4)   // Local Vector Table 0 (TIMER)
  #define X1         0x0000000B   // divide counts by 1
  #define PERIODIC   0x00020000   // Periodic
#define PCINT   (0x0340/4)   // Performance Counter LVT
#define LINT0   (0x0350/4)   // Local Vector Table 1 (LINT0)
#define LINT1   (0x0360/4)   // Local Vector Table 2 (LINT1)
#define ERROR   (0x0370/4)   // Local Vector Table 3 (ERROR)
  #define MASKED     0x00010000   // Interrupt masked
#define TICR    (0x0380/4)   // Timer Initial Count
#define TCCR    (0x0390/4)   // Timer Current Count
#define TDCR    (0x03E0/4)   // Timer Divide Configuration

volatile uint32_t *lapic;

static void lapicw(uint64_t index, uint64_t value) {
    lapic[index] = value;
    lapic[ID];
}

void lapic_init(void) {
    uint64_t apic_base = rdmsr(IA32_APIC_BASE_MSR);
    if( !(apic_base & IA32_APIC_BASE_MSR_ENABLE) ){
        wrmsr(IA32_APIC_BASE_MSR, apic_base | IA32_APIC_BASE_MSR_ENABLE);
    }

    apic_base &= ~0xFFFULL;

    lapic = io_remap((void*)apic_base, PGSIZE_4KB);
    
    lapicw(SVR, ENABLE | (T_IRQ0 + IRQ_SPURIOUS));

    lapicw(TDCR, X1);
    lapicw(TIMER, PERIODIC | (T_IRQ0 + IRQ_TIMER));
    lapicw(TICR, 10000000);

    lapicw(LINT0, MASKED);
    lapicw(LINT1, MASKED);

    if(((lapic[VER]>>16) & 0xFF) >= 4)
        lapicw(PCINT, MASKED);

    lapicw(ERROR, T_IRQ0 + IRQ_ERROR);

    lapicw(ESR, 0);
    lapicw(ESR, 0);

    lapicw(EOI, 0);

    // lapicw(ICRHI, 0);
    // lapicw(ICRLO, BCAST |INIT | LEVEL);
    // while(lapic[ICRLO] & DELIVS); 
    // single core first

    lapicw(TPR,0);

}

void lapic_eoi(void) {
    if(lapic) lapicw(EOI, 0);
}