#include "sigscanner.h"

#include <stdio.h>
#include <string.h>

static unsigned char *byteCache; // Windows module copied in

static inline bool Compare( const unsigned char* pData, const unsigned char* bMask, const char* szMask )
{
    for (; *szMask; ++szMask, ++pData, ++bMask)
        if (*szMask == 'x' && *pData != *bMask)
            return false;

    return (*szMask) == 0;
}

static uintptr_t FindPattern( uintptr_t start, uintptr_t max, unsigned char* bMask, const char* szMask )
{
    for (uintptr_t i = start; i < max; i++) {
        if ( Compare( (unsigned char*)i, bMask, szMask ) ){
            return i;
        }
    }

    return 0;
}

Address FindPatternInMemory( VirtualMemoryObj *memory, const char *pattern, Address start, Address size )
{
    uintptr_t ret = 0;
    size_t patternLen = strlen( pattern );
    unsigned char *bytes = malloc( size ); // NOTE: the entire region is copied into linux-memory!
    if( !bytes )
    {
        printf("Failed to malloc %d bytes!\n", size);
        return 0;
    }

    //these are a bit bigger than needed.
    unsigned char *bMask = malloc( patternLen );
    char *szMask = malloc( patternLen );
    memset(bMask, 0, patternLen);
    memset(szMask, 0, patternLen);

    char byteBuffer[3] = { '\0', '\0', '\0' };
    unsigned int bMaskIndex = 0;
    unsigned int szMaskIndex = 0;

    // read the memory into linux
    virt_read_raw_into(memory, start, bytes, size);

    // Generate a byte mask and x/? mask at the same time
    for( size_t i = 0; i < patternLen; i++ ){
        if( pattern[i] == ' ' ){
            szMaskIndex++;
            continue;
        }
        if( pattern[i] == '?' ) {
            szMask[szMaskIndex] = '?';
        } else {
            szMask[szMaskIndex] = 'x';
        }

        // End of word
        if( pattern[i+1] == ' ' || pattern[i+1] == '\0' ){
            byteBuffer[0] = pattern[i - 1];
            byteBuffer[1] = pattern[i];
            bMask[bMaskIndex] = (unsigned char)strtoul( byteBuffer, 0, 16 );
            bMaskIndex++;
        }
    }

    // FindPattern returns pointer to pattern.
    uintptr_t offset = FindPattern( (uintptr_t)bytes, (uintptr_t)&bytes[size], bMask, szMask );
    if( !offset )
    {
        printf("FindPattern failed! (%s)\n", szMask);
        ret = 0;
        goto end;
    }

    // convert to offset and then add offset to original address.
    offset = offset - (uintptr_t)bytes;
    ret = start + offset;

end:
    free( bMask );
    free( szMask );
    free( bytes );

    return ret;
}

static uintptr_t FindPatternOffset( uintptr_t start, uintptr_t max, unsigned char* bMask, const char* szMask )
{
    for (uintptr_t i = start; i < max; i++) {
        if ( Compare( (unsigned char*)i, bMask, szMask ) ){
            return i;
        }
    }

    return 0;
}