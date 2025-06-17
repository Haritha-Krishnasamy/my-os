#include "port.h"
#include "types.h"

// Base class : Port

Port::Port(uint16_t portnumber)
{
    this->portnumber = portnumber;
}

Port::~Port()
{
}

// Derived class : 8-bit Port

Port8bit::Port8bit(uint16_t portnumber)
    : Port(portnumber)
{
}

Port8bit::~Port8bit()
{
}

// 8-bit Port : Write implementation

void Port8bit::Write(uint8_t data)
{
    __asm__ volatile("outb %0, %1" : : "a"(data), "Nd"(portnumber));
}

// 8-bit Port : Read implementation

uint8_t Port8bit::Read()
{
    uint8_t result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(portnumber));
    return result;
}

// Derived from 8-bit Port : Slow 8-bit Port

Port8bitslow::Port8bitslow(uint16_t portnumber)
    : Port8bit(portnumber)
{
}

Port8bitslow::~Port8bitslow()
{
}

// 8-bit Port : Write implementation

void Port8bitslow::Write(uint8_t data)
{
    // Add dummy jumps to add delays in write operation
    __asm__ volatile(
        "outb %0, %1\n"
        "jmp 1f\n"
        "1: jmp 1f\n"
        "1:"
        :
        : "a"(data), "Nd"(portnumber));
}

// 16-bit Port

Port16bit::Port16bit(uint16_t portnumber)
    : Port(portnumber)
{
}

Port16bit::~Port16bit()
{
}

// 16-bit Port : Write implementation

void Port16bit::Write(uint16_t data)
{
    __asm__ volatile("outw %0, %1" : : "a"(data), "Nd"(portnumber));
}

uint16_t Port16bit::Read()
{
    uint16_t result;
    __asm__ volatile("inw %1, %0" : "=a"(result) : "Nd"(portnumber));
    return result;
}

// Derived port : 32-bit Port implementation

Port32bit::Port32bit(uint16_t portnumber)
    : Port(portnumber)
{
}

Port32bit::~Port32bit()
{
}

// 32-bit Port : Write implementation

void Port32bit::Write(uint32_t data)
{
    __asm__ volatile("outl %0, %1" : : "a"(data), "Nd"(portnumber));
}

// 32-bit Port : Read implementation

uint32_t Port32bit::Read()
{
    uint32_t result;
    __asm__ volatile("inl %1, %0" : "=a"(result) : "Nd"(portnumber));
    return result;
}