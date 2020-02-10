#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include "../lib/weighted-bipartite-perfect-matching/include/hungarian.h"
#include "../lib/XORSATFilter/include/xorsat_filter.h"
#include "../lib/c_list_types/include/c_list_types.h"

#include "mphf_hashes.h"
#include "mphf_blocks.h"

//Comment out the following to silence progress updates in `MPHFBuilderFinalize`
#define MPHF_PRINT_BUILD_PROGRESS

typedef struct MPHFParameters {
  uint16_t nEltsPerBlock; //  Large numbers provide higher efficiency
                          //  Small numbers provide faster build time
  XORSATFilterParameters xsfp; //Parameters for the XORSAT Filter  
} MPHFParameters;

extern MPHFParameters MPHFEfficientParameters;
extern MPHFParameters MPHFPaperParameters;
extern MPHFParameters MPHFFastParameters;

typedef struct MPHFBuilder {
  MPHFHash_list pHashes;
  MPHFBlock_list pBlocks;
} MPHFBuilder;

typedef struct MPHFQuerier {
  uint32_t nBlocks;
  int16_t *pOffsets;
  uint16_t nAvgVarsPerBlock;
  XORSATFilterQuerier *xsfq;
} MPHFQuerier;

#include "mphf_serial.h"

MPHFBuilder *MPHFBuilderAlloc(uint32_t nExpectedElements);
void MPHFBuilderFree(MPHFBuilder *mphfb);
uint8_t MPHFBuilderAddElement(MPHFBuilder *mphfb, const void *pElement, size_t nElementBytes);
MPHFQuerier *MPHFBuilderFinalize(MPHFBuilder *mphfb, MPHFParameters sParams, uint32_t nThreads);
MPHFQuerier *MPHFCreateQuerierFromBuilder(MPHFBuilder *mphfb, uint32_t nMPHFElements, MPHFParameters sParams, uint32_t nThreads);
uint32_t MPHFQuery(MPHFQuerier *mphfq, const void *pElement, size_t nElementBytes);
void MPHFQuerierFree(MPHFQuerier *mphfq);
uint32_t MPHFQueryRate(MPHFQuerier *mphfq);
uint64_t MPHFSize(MPHFQuerier *mphfq);
