#ifndef __PORT_H
#define __PORT_H

#include "types.h"

// Port - Abstract base class

class Port
{
protected:
    uint16_t portnumber;
    Port(uint16_t portnumber);
    ~Port();
};

// 8-bit Port : Derived from Port class

class Port8bit : public Port
{
public:
    Port8bit(uint16_t portnumber);
    ~Port8bit();

    virtual void Write(uint8_t data);
    virtual uint8_t Read();
};

// 8-bit slow Port : Derived from 8-bit Port

class Port8bitslow : public Port8bit
{
public:
    Port8bitslow(uint16_t portnumber);
    ~Port8bitslow();

    // Delay induced in Write
    virtual void Write(uint8_t data);

    // Read function inherited from 8-bit Port
};

// 16-bit Port : Derived from Port class

class Port16bit : public Port
{
public:
    Port16bit(uint16_t portnumber);
    ~Port16bit();

    virtual void Write(uint16_t data);
    virtual uint16_t Read();
};

// 32-bit Port : Derived from Port class

class Port32bit : public Port
{
public:
    Port32bit(uint16_t portnumber);
    ~Port32bit();

    virtual void Write(uint32_t data);
    virtual uint32_t Read();
};

#endif