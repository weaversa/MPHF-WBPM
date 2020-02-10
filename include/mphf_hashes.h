#ifndef MPHF_HASHES
#define MPHF_HASHES

#include "../lib/XORSATFilter/include/MurmurHash3.h"

#define XXH_INLINE_ALL
#define XXH_STATIC_LINKING_ONLY   /* access advanced declarations */
#define XXH_IMPLEMENTATION   /* access definitions */
#include "../lib/XORSATFilter/lib/xxHash/xxhash.h"
//#include "../lib/xxHash/xxh3.h"

typedef struct MPHFHash {
  uint64_t h1;
} MPHFHash;

create_c_list_headers(MPHFHash_list, MPHFHash)

MPHFHash MPHFGenerateHashesFromElement(const void *pElement, size_t nElementBytes);
uint32_t MPHFHashToBlock(MPHFHash mphfh, uint32_t nBlocks);
uint32_t MPHFGenerateIthValueFromHash(MPHFHash mphfh, uint32_t nVariables, uint32_t i);

#endif
