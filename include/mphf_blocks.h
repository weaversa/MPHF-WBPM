#ifndef MPHFBLOCK_H
#define MPHFBLOCK_H

typedef struct MPHFBlock {
  MPHFHash_list pHashes;
  uint32_t_list pMatching;
  uint32_t nThreadNumber;
} MPHFBlock;

create_c_list_headers(MPHFBlock_list, MPHFBlock)

void MPHFBlockFree(MPHFBlock *pBlock);
uint8_t MPHFBuildBlock(MPHFBlock *pBlock);

#endif
