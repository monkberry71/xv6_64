#include <stdint.h>
#include "../io.h"
#define COM1 0x3F8

// https://wiki.osdev.org/Serial_Ports

int init_serial() {
   outb(COM1 + 1, 0x00);    // Disable all interrupts
   outb(COM1 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
   outb(COM1 + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
   outb(COM1 + 1, 0x00);    //                  (hi byte)
   outb(COM1 + 3, 0x03);    // 8 bits, no parity, one stop bit
   outb(COM1 + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
   outb(COM1 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
   outb(COM1 + 4, 0x1E);    // Set in loopback mode, test the serial chip
   outb(COM1 + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

   // Check if serial is faulty (i.e: not same byte as sent)
   if(inb(COM1 + 0) != 0xAE) {
      return 1;
   }

   // If serial is not faulty set it in normal operation mode
   // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
   outb(COM1 + 4, 0x0F);
   return 0;
}

int is_transmit_empty() {
   return inb(COM1 + 5) & 0x20;
}

void serial_putc(char a) {
   while (is_transmit_empty() == 0);

   outb(COM1,a);
}
void serial_puts(const char *s) {
   while (*s) serial_putc(*s++);
}

void serial_hex(uint64_t v) {
   char buf[17];
   buf[16] = 0;
   for(int i = 15; i >= 0; i--) {
      buf[i] = "0123456789abcdef"[v & 0xf];
      v >>= 4;
   }
   serial_puts(buf);
}

// void serial_init(void) {
//     outb(COM1 + 1, 0x00); // disable interrupts
//     outb(COM1 + 3, 0x80); // enable DLAB
//     outb(COM1 + 0, 0x03); // baud divisor low: 38400
//     outb(COM1 + 1, 0x00); // baud divisor high
//     outb(COM1 + 3, 0x03); // 8 bits, no parity, 1 stop bit
//     outb(COM1 + 2, 0xC7); // enable FIFO
// }

// void serial_putc(char c) {
//     while (!(inb(COM1 + 5) & 0x20));
//     outb(COM1, c);
// }
