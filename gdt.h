#ifndef __GDT_H
#define __GDT_H

#include "types.h"

class GlobalDescriptorTable
{

public:
    class SegmentDescriptor
    {
    private:
        uint16_t limit_lo;
        uint16_t base_lo;
        uint8_t base_hi;
        uint8_t type;
        uint8_t flags_limit_hi;
        uint8_t base_vhi;

    public:
        SegmentDescriptor(uint32_t base, uint32_t limit, uint8_t type);
        uint32_t Base();
        uint32_t Limit();

    } __attribute__((packed));

    SegmentDescriptor null_SegmentSelector;
    SegmentDescriptor unused_SegmentSelector;
    SegmentDescriptor code_SegmentSelector;
    SegmentDescriptor data_SegmentSelector;

public:
    GlobalDescriptorTable();
    ~GlobalDescriptorTable();

    uint16_t Code_SegmentSelector();
    uint16_t Data_SegmentSelector();
};

#endif