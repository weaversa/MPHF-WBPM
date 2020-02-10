#include "mphf.h"

uint8_t MPHFSerialize(FILE *pMPHFFile, MPHFQuerier *mphfq) {
  if(pMPHFFile == NULL) return 1; //Failure
  
  size_t write;

  XORSATFilterQuerier *xsfq = mphfq->xsfq;
  
  uint64_t nFilterWords = XORSATFilterGetBlockIndex(xsfq, xsfq->nBlocks);

  //Write mphf block offsets
  write = fwrite(mphfq->pOffsets, sizeof(uint16_t), mphfq->nBlocks, pMPHFFile);
  if(write != mphfq->nBlocks) return 1; //Failure

  //Write filter
  write = fwrite(xsfq->pFilter, sizeof(uint64_t), nFilterWords, pMPHFFile);
  if(write != nFilterWords) return 1; //Failure

  //Write filter block offsets
  write = fwrite(xsfq->pOffsets, sizeof(int16_t), xsfq->nBlocks+1, pMPHFFile);
  if(write != (xsfq->nBlocks+1)) return 1; //Failure

  //Write filter header
  XORSATFilterSerialData xsfsd = {.nBlocks = xsfq->nBlocks,
				  .nAvgVarsPerBlock = xsfq->nAvgVarsPerBlock,
				  .nSolutions = xsfq->nSolutions,
				  .nMetaDataBytes = xsfq->nMetaDataBytes,
				  .nLitsPerRow = xsfq->nLitsPerRow};
  MPHFSerialData mphfsd = { .nBlocks = mphfq->nBlocks,
			    .nAvgVarsPerBlock = mphfq->nAvgVarsPerBlock,
			    .xsfsd = xsfsd};
    
  write = fwrite(&mphfsd, sizeof(MPHFSerialData), 1, pMPHFFile);
  if(write != 1) return 1; //Failure

  return 0; //Success
}

MPHFQuerier *MPHFDeserialize(FILE *pMPHFFile) {
  if(pMPHFFile == NULL) return NULL;
  
  int seek = fseek(pMPHFFile, -(int32_t)(sizeof(uint16_t) + sizeof(MPHFSerialData)), SEEK_END);
  if(seek != 0) return NULL;

  //Record the file size (minus the header data) for later verification
  uint64_t nDataSize = (uint64_t) ftell(pMPHFFile) + sizeof(uint16_t);
  
  size_t read;

  //Read the last filter block offset (needed to compute the size of the filter)
  int16_t nDiff;
  read = fread(&nDiff, sizeof(int16_t), 1, pMPHFFile);
  if(read != 1) return NULL;  

  //Read filter header
  MPHFSerialData mphfsd;
  read = fread(&mphfsd, sizeof(MPHFSerialData), 1, pMPHFFile);
  if(read != 1) return NULL;

  XORSATFilterSerialData xsfsd = mphfsd.xsfsd;
  
  if(xsfsd.nSolutions > 32) {
    fprintf(stderr, "Error: nSolutions must be <= 32\n"); //For now
    return NULL;
  }

  XORSATFilterQuerier *xsfq = (XORSATFilterQuerier *)malloc(1 * sizeof(XORSATFilterQuerier));
  if(xsfq == NULL) return NULL;

  xsfq->nBlocks = xsfsd.nBlocks;
  xsfq->nAvgVarsPerBlock = xsfsd.nAvgVarsPerBlock;
  xsfq->nSolutions = xsfsd.nSolutions;
  xsfq->nMetaDataBytes = xsfsd.nMetaDataBytes;
  xsfq->nLitsPerRow = xsfsd.nLitsPerRow;

  //Compute the size of the filter
  //See xorsat_query::XORSATFilterGetBlockIndex for more info
  int64_t nExpectedIndex = ((int64_t) xsfq->nAvgVarsPerBlock) * (int64_t) xsfq->nBlocks;
  //Round up to next multiple of 64
  nExpectedIndex = ((nExpectedIndex-1) | (int64_t) 0x3f) + (int64_t) 1;
  uint64_t nFilterWords = (uint64_t) (((nExpectedIndex - ((int64_t) nDiff * 64)) >> 6) * (int64_t) (xsfq->nSolutions + (xsfq->nMetaDataBytes*8)));

  if(nDataSize != (mphfsd.nBlocks * sizeof(int16_t)) + (nFilterWords * sizeof(uint64_t)) + ((xsfq->nBlocks+1) * sizeof(int16_t))) {
    fprintf(stderr, "Error: filter file is corrupt %lu vs %lu\n", nDataSize, (mphfsd.nBlocks * sizeof(int16_t)) + (nFilterWords * sizeof(uint64_t)) + ((xsfq->nBlocks+1) * sizeof(int16_t)));
    free(xsfq);
    return NULL;
  }
  
  MPHFQuerier *mphfq = (MPHFQuerier *)malloc(1 * sizeof(MPHFQuerier));
  if(mphfq == NULL) return NULL;
  mphfq->nBlocks = mphfsd.nBlocks;
  mphfq->nAvgVarsPerBlock = mphfsd.nAvgVarsPerBlock;
  mphfq->xsfq = xsfq;
  
  //Read mphf block offsets, filter, and filter block offsets
  mphfq->pOffsets = (int16_t *)mmap(0, (mphfq->nBlocks * sizeof(int16_t)) + (nFilterWords * sizeof(uint64_t)) + ((xsfq->nBlocks+1) * sizeof(int16_t)), PROT_READ, MAP_PRIVATE, fileno(pMPHFFile), 0);
  xsfq->pFilter = (uint64_t *) (mphfq->pOffsets + mphfq->nBlocks);

  xsfq->pOffsets = (int16_t *) (xsfq->pFilter + nFilterWords);

  xsfq->bMMAP = 1;

  return mphfq;
}
