#include "mphf.h"

MPHFQuerier *MPHFQuerierAlloc(uint32_t nBlocks) {
  MPHFQuerier *mphfq = (MPHFQuerier *)malloc(1 * sizeof(MPHFQuerier));
  mphfq->nBlocks = nBlocks;
  mphfq->pOffsets = (int16_t *)malloc(nBlocks * sizeof(int16_t));
  mphfq->xsfq = NULL;
  
  return mphfq;
}

void MPHFQuerierFree(MPHFQuerier *mphfq) {
  if(mphfq != NULL) {

    if(mphfq->pOffsets != NULL) {
      if(mphfq->xsfq->bMMAP == 1) {
	uint64_t nFilterWords = XORSATFilterGetBlockIndex(mphfq->xsfq, mphfq->xsfq->nBlocks);
	munmap(mphfq->pOffsets, (mphfq->nBlocks * sizeof(int16_t)) + (nFilterWords * sizeof(uint64_t)) + (((uint64_t) mphfq->xsfq->nBlocks+1) * sizeof(int16_t)));
	mphfq->xsfq->pFilter = NULL;
	mphfq->xsfq->pOffsets = NULL;
      } else     
	free(mphfq->pOffsets);
    }

    if(mphfq->xsfq != NULL) {
      XORSATFilterQuerierFree(mphfq->xsfq);
    }
    
    free(mphfq);
  }
}

MPHFQuerier *MPHFCreateQuerierFromBuilder(MPHFBuilder *mphfb, uint32_t nMPHFElements, MPHFParameters sParams, uint32_t nThreads) {
  uint32_t i, j;
  uint32_t nBlock;
  
  MPHFQuerier *mphfq = MPHFQuerierAlloc((uint32_t) mphfb->pBlocks.nLength);

  //Calculate offsets
  uint16_t nAvgElements = (uint16_t) (nMPHFElements / (uint32_t) mphfb->pBlocks.nLength);
  mphfq->nAvgVarsPerBlock = nAvgElements;

  int32_t nCurrElements = 0;
  for(nBlock = 0; nBlock < mphfb->pBlocks.nLength; nBlock++) {
    MPHFBlock *pBlock = &mphfb->pBlocks.pList[nBlock];
    nCurrElements += (int32_t) pBlock->pMatching.nLength;
    int32_t nExpectedElements = (int32_t) (nAvgElements * (nBlock+1));
    int32_t pOffset = nExpectedElements - nCurrElements;
    assert(pOffset <= 32767 && pOffset >= -32768);
    mphfq->pOffsets[nBlock] = (int16_t)pOffset;
  }

  //Count number of filter elements
  uint32_t nFilterElements = 0;
  for(nBlock = 0; nBlock < mphfb->pBlocks.nLength; nBlock++) {
    MPHFBlock *pBlock = &mphfb->pBlocks.pList[nBlock];
    uint32_t n = (uint32_t) pBlock->pMatching.nLength;
    uint32_t *pMatching = pBlock->pMatching.pList;

    for(i = 0; i < n; i++) {
      MPHFHash mphfh = pBlock->pHashes.pList[i];
      for(j = 0; 1; j++) {
	nFilterElements++;
        uint32_t key = MPHFGenerateIthValueFromHash(mphfh, n, j);
        if(pMatching[i] == key) break;
      }
    }
  }

  //Build XORSAT Filter
  XORSATFilterBuilder *xsfb = XORSATFilterBuilderAlloc(nFilterElements, 0);
  
  for(nBlock = 0; nBlock < mphfb->pBlocks.nLength; nBlock++) {
    MPHFBlock *pBlock = &mphfb->pBlocks.pList[nBlock];
    uint32_t n = (uint32_t) pBlock->pMatching.nLength;
    uint32_t *pMatching = pBlock->pMatching.pList;

    for(i = 0; i < n; i++) {
      MPHFHash mphfh = pBlock->pHashes.pList[i];
      for(j = 0; 1; j++) {
        uint32_t key = MPHFGenerateIthValueFromHash(mphfh, n, j);
        uint64_t hash_i = mphfh.h1+(uint64_t)j;
        if(pMatching[i] == key) {
          XORSATFilterBuilderAddElement(xsfb, &hash_i, 8, NULL);
          break;
        } else {
          XORSATFilterBuilderAddAbsence(xsfb, &hash_i, 8);
        }
      }
    }

    MPHFBlockFree(pBlock);
  }

#ifdef MPHF_PRINT_BUILD_PROGRESS
  assert(nFilterElements == (uint32_t) xsfb->pHashes.nLength);
  fprintf(stderr, "Minimum weight / num elements is %4.3lf\n", ((double)nFilterElements) / (double)nMPHFElements);
#endif

  mphfq->xsfq =
    XORSATFilterBuilderFinalize(xsfb,
                                sParams.xsfp,
                                nThreads);
  
  XORSATFilterBuilderFree(xsfb);

#ifdef MPHF_PRINT_BUILD_PROGRESS  
  fprintf(stdout, "Testing filter efficiency with util func: %4.2lf%% efficient\n", XORSATFilterEfficiency(mphfq->xsfq, nFilterElements, 0.5) * 100.0);
  fprintf(stdout, "Filter uses %"PRIu64" bits, %4.2lf bits per element\n", XORSATFilterSize(mphfq->xsfq), ((double) XORSATFilterSize(mphfq->xsfq)) / (double) nFilterElements);
#endif
  
  return mphfq;
}

uint32_t MPHFQuery(MPHFQuerier *mphfq, const void *pElement, size_t nElementBytes) {
  uint32_t j;
  
  MPHFHash mphfh = MPHFGenerateHashesFromElement(pElement, nElementBytes);
  uint32_t nBlock = MPHFHashToBlock(mphfh, mphfq->nBlocks);

  uint32_t nOffset0 = nBlock == 0 ? 0 : (uint32_t) mphfq->pOffsets[nBlock-1];
  uint32_t nOffset1 = (uint32_t) mphfq->pOffsets[nBlock];
  uint32_t nVarsInBlock = (uint32_t) mphfq->nAvgVarsPerBlock + nOffset0 - nOffset1;
  
  uint32_t nKey;
  for(j = 0; 1; j++) {
    uint64_t hash_i = mphfh.h1+(uint64_t)j;
    if(XORSATFilterQuery(mphfq->xsfq, &hash_i, 8) == 1) {
      nKey = MPHFGenerateIthValueFromHash(mphfh, nVarsInBlock, j);
      break;
    }
  }

  uint32_t nExpectedVars = mphfq->nAvgVarsPerBlock * nBlock;
  uint32_t nNumVarsSoFar = nExpectedVars - nOffset0;
  
  return nKey + nNumVarsSoFar;
}


uint32_t MPHFQueryRate(MPHFQuerier *mphfq) {
  uint32_t i;
  uint32_t nElementsQueried = 10000000;

  clock_t start = clock();
  for(i = 0; i < nElementsQueried; i++) {
    uint32_t volatile ret = MPHFQuery(mphfq, &i, sizeof(uint32_t));
  }
  clock_t end = clock();
  
  double time_elapsed_in_seconds = ((double) (end - start)) / (double) CLOCKS_PER_SEC;
  return (uint32_t) (((double) nElementsQueried) / time_elapsed_in_seconds);
}

uint64_t MPHFSize(MPHFQuerier *mphfq) {
  uint64_t nXORSATFilterBits = XORSATAncillarySize(mphfq->xsfq) + XORSATFilterSize(mphfq->xsfq) + XORSATMetaDataSize(mphfq->xsfq);
  uint64_t nMPHFBits = nXORSATFilterBits + 32 + (mphfq->nBlocks * 16) + 16;
  
  return nMPHFBits;
}
