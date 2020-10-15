#ifndef JIAA_SIGSCANNER_H
#define JIAA_SIGSCANNER_H

#include "memflow_win32.h"

// NOTE: the entire region is copied into linux-memory!
Address FindPatternInMemory( VirtualMemoryObj *memory, const char *pattern, Address start, Address size );

static uintptr_t GetAbsoluteAddressVm( VirtualMemoryObj *memory, uintptr_t instructionPtr, int offset, int size )
{
    if( !instructionPtr )
        return 0;

    return instructionPtr + virt_read_u32(memory, (instructionPtr + offset)) + size;
}

#endif //JIAA_SIGSCANNER_H
