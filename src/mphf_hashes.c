#include "mphf.h"

create_c_list_type(MPHFHash_list, MPHFHash)

inline
MPHFHash MPHFGenerateHashesFromElement(const void *pElement, size_t nElementBytes) {
  MPHFHash mphfh;
  uint64_t nonce = 0;
  assert(nElementBytes < 0x7fffffff);

  do {
    mphfh.h1 = XXH3_64bits_withSeed(pElement, nElementBytes, (unsigned long long)0xa7cb5c58462b6c85 + (nonce++));
  } while (mphfh.h1 == 0);
  
  return mphfh;
}

inline
uint32_t MPHFHashToBlock(MPHFHash mphfh, uint32_t nBlocks) {
  uint64_t h1 = mphfh.h1 * 0x1ae202980e70d8f1;
  return mphfh.h32[1] % nBlocks;
}

//Here we use the Kirsch-Mitzenmacher technique.
inline
uint32_t MPHFGenerateIthValueFromHash(MPHFHash mphfh, uint32_t nVariables, uint32_t i) {
  return (mphfh.h32[0] + (mphfh.h32[1] * i)) % nVariables;
}
