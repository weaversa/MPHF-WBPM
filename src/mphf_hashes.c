#include "mphf.h"

create_c_list_type(MPHFHash_list, MPHFHash)

inline
MPHFHash MPHFGenerateHashesFromElement(const void *pElement, size_t nElementBytes) {
  MPHFHash mphfh;
  uint64_t nonce = 0;
  assert(nElementBytes < 0x7fffffff);

  do {
    mphfh.h1 = XXH64(pElement, nElementBytes, (unsigned long long)0xa7cb5c58462b6c85 + (nonce++));
    //mphfh.h2 = XXH64(pElement, nElementBytes, (unsigned long long)mphfh.h1);
    //mphfh.h2 = mphfh.h1 * (unsigned long long)0x1ae202980e70d8f1; //Slightly faster
  } while (mphfh.h1 == 0);
  
  return mphfh;
}

inline
uint32_t MPHFHashToBlock(MPHFHash mphfh, uint32_t nBlocks) {
  //mphfh.h2 = mphfh.h1 * (unsigned long long)0x1ae202980e70d8f1; //Slightly faster
  //return (((uint32_t *)&xsfh)[0] ^ 0x99ce5d13) % nBlocks;
  uint64_t h1 = mphfh.h1 * 0x1ae202980e70d8f1; //Slightly faster
  return ((uint32_t *)&h1)[1] % nBlocks;
  //return ((uint32_t *)&mphfh)[3] % nBlocks;
}

//Here we use the Kirsch-Mitzenmacher technique.
inline
uint32_t MPHFGenerateIthValueFromHash(MPHFHash mphfh, uint32_t nVariables, uint32_t i) {
  uint32_t *mphfh_32 = (uint32_t *)&mphfh;
  return (mphfh_32[0] + (mphfh_32[1] * i)) % nVariables;
}
