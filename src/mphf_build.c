#include "mphf.h"

/*  nExpectedElements can be 0 */
MPHFBuilder *MPHFBuilderAlloc(uint32_t nExpectedElements) {
  MPHFBuilder *mphfb = (MPHFBuilder *)malloc(1 * sizeof(MPHFBuilder));
  if(mphfb == NULL) return NULL;

  if(MPHFHash_list_init(&mphfb->pHashes, nExpectedElements) != C_LIST_NO_ERROR) {
    free(mphfb);
    return NULL;
  }

  if(MPHFBlock_list_init(&mphfb->pBlocks, 0) != C_LIST_NO_ERROR) {
    MPHFHash_list_free(&mphfb->pHashes, NULL);
    free(mphfb);
    return NULL;
  }
  
  return mphfb;
}

void MPHFBuilderFree(MPHFBuilder *mphfb) {
  MPHFHash_list_free(&mphfb->pHashes, NULL);
  MPHFBlock_list_free(&mphfb->pBlocks, MPHFBlockFree);
  free(mphfb);
}

uint8_t MPHFBuilderAddElement(MPHFBuilder *mphfb, const void *pElement, size_t nElementBytes) {
  MPHFHash pHash = MPHFGenerateHashesFromElement(pElement, nElementBytes);
  return MPHFHash_list_push(&mphfb->pHashes, pHash);
}

uint8_t MPHFDistributeHashesToBlocks(MPHFBuilder *mphfb, MPHFParameters sParams);

MPHFQuerier *MPHFBuilderFinalize(MPHFBuilder *mphfb, MPHFParameters sParams, uint32_t nThreads) {
  uint8_t ret;
  uint32_t i;

  //Sanity check parameters
  if(sParams.nEltsPerBlock > mphfb->pHashes.nLength) {
    sParams.nEltsPerBlock = (uint16_t) mphfb->pHashes.nLength;
  }

  uint32_t nMPHFElements = (uint32_t) mphfb->pHashes.nLength;
  
  ret = MPHFDistributeHashesToBlocks(mphfb, sParams);
  if(ret != 0) return NULL;

  uint32_t max_offset = 0;
  uint16_t nAvgElements = sParams.nEltsPerBlock;
  int32_t nCurrElements = 0;
  uint32_t nBlock;
  for(nBlock = 0; nBlock < mphfb->pBlocks.nLength; nBlock++) {
    MPHFBlock *pBlock = &mphfb->pBlocks.pList[nBlock];
    nCurrElements += (int32_t) pBlock->pHashes.nLength;
    int32_t nExpectedElements = (int32_t) (nAvgElements * (nBlock+1));
    int32_t pOffset = nExpectedElements - nCurrElements;
    if(pOffset > 32767 || pOffset < -32768) {
      fprintf(stderr, "MPHF building not possible. Some offset delta is %d which is more than fits in 16 bits. Decrease number of elements.\n", pOffset);
      return NULL;
    }
    if(max_offset < abs(pOffset))
      max_offset = abs(pOffset);
  }

  fprintf(stderr, "max offset = %u\n", max_offset);
  
   //Build Blocks
  threadpool thpool = thpool_init(nThreads);
  for(i = 0; i < mphfb->pBlocks.nLength; i++) {
    MPHFBlock *pBlock = &mphfb->pBlocks.pList[i];
    thpool_add_work(thpool, (void*)MPHFBuildBlock, pBlock);
  }

  thpool_wait(thpool);
  thpool_destroy(thpool);
  
  MPHFQuerier *mphfq = MPHFCreateQuerierFromBuilder(mphfb, nMPHFElements, sParams, nThreads);

  return mphfq;
}
