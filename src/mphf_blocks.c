#include "mphf.h"

// A brief description of the parameters below is given in `include/mphf.h`.

MPHFParameters MPHFEfficientParameters =
  { .nEltsPerBlock = 1500,
    .xsfp =
    { .nLitsPerRow   = 6,
      .nSolutions    = 1,
      .nEltsPerBlock = 5000,
      .fEfficiency   = 1.00 },
  };
 
MPHFParameters MPHFPaperParameters =
  { .nEltsPerBlock = 12288,
    .xsfp =
    { .nLitsPerRow   = 0,
      .nSolutions    = 1,
      .nEltsPerBlock = 4608, //12288,//6144,
      .fEfficiency   = 1.00 },
  };

MPHFParameters MPHFFastParameters =
  { .nEltsPerBlock = 100,
    .xsfp = 
    { .nLitsPerRow   = 4,
      .nSolutions    = 1,
      .nEltsPerBlock = 750,
      .fEfficiency   = 0.95 },
  };

create_c_list_type(MPHFBlock_list, MPHFBlock)

void MPHFBlockAlloc(MPHFBlock *pBlock, uint32_t nVariablesPerBlock) {
  MPHFHash_list_init(&pBlock->pHashes, nVariablesPerBlock);
  pBlock->nThreadNumber = 0;
}

void MPHFBlockFree(MPHFBlock *pBlock) {
  if(pBlock->pHashes.pList != NULL)
    MPHFHash_list_free(&pBlock->pHashes, NULL);
  
  if(pBlock->pMatching.pList != NULL)
    uint32_t_list_free(&pBlock->pMatching, NULL);
}

uint8_t MPHFDistributeHashesToBlocks(MPHFBuilder *mphfb, MPHFParameters sParams) {
  uint32_t i;
  uint32_t nBlocks;
  
  //Determine number of blocks
  nBlocks = ((uint32_t) mphfb->pHashes.nLength) / (uint32_t) sParams.nEltsPerBlock;

#ifdef MPHF_PRINT_BUILD_PROGRESS
  fprintf(stderr, "%u blocks, roughly %u variables per block\n", nBlocks, sParams.nEltsPerBlock);
#endif
  
  //Initialize blocks
  uint8_t ret = MPHFBlock_list_resize(&mphfb->pBlocks, nBlocks);
  if(ret != C_LIST_NO_ERROR) return ret;
  
  mphfb->pBlocks.nLength = nBlocks;
  for(i = 0; i < nBlocks; i++) {
    MPHFBlockAlloc(&mphfb->pBlocks.pList[i], sParams.nEltsPerBlock);
  }
  
  //Distribute elements over blocks
  for(i = 0; i < mphfb->pHashes.nLength; i++) {
    MPHFHash pHash = mphfb->pHashes.pList[i];
    uint32_t nBlock = MPHFHashToBlock(pHash, nBlocks);
    MPHFBlock *pBlock = &mphfb->pBlocks.pList[nBlock];
    ret = MPHFHash_list_push(&pBlock->pHashes, pHash);
    if(ret != C_LIST_NO_ERROR) return ret;
  }

  MPHFHash_list_free(&mphfb->pHashes, NULL);

  return 0;
}

uint8_t MPHFBuildBlock(MPHFBlock *pBlock) {
  uint32_t i, j;
  uint32_t n = (uint32_t) pBlock->pHashes.nLength;

  uint32_t k = (uint32_t) ceil(log(n) + log(log(n)));
  k = k >= 3 ? k : 3; //Or, better yet, keep going until every RHS has
  //been generated twice.

  while(1) {
    WeightedBipartiteEdge *edges = malloc((n * k) * sizeof(WeightedBipartiteEdge));

    uint32_t e = 0;
    
    for(j = 0; j < k; j++) {
      for(i = 0; i < n; i++) {
        MPHFHash mphfh = pBlock->pHashes.pList[i];
        edges[e].left = i;
        edges[e].right = MPHFGenerateIthValueFromHash(mphfh, n, j);
        edges[e].cost = (int32_t) j;
        e++;
      }
    }
    
    pBlock->pMatching.pList = hungarianMinimumWeightPerfectMatching(n, edges, e);
    pBlock->pMatching.nLength = n;
    pBlock->pMatching.nLength_max = n;

    free(edges);
    
    if(pBlock->pMatching.pList == NULL) {
      k++;
    } else {
      break;
    }
  }

  return 0;
}
