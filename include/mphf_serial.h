#ifndef MPHFSERIAL_H
#define MPHFSERIAL_H

typedef struct MPHFSerialData {
  //MPHF Data
  uint32_t nBlocks;
  uint16_t nAvgVarsPerBlock;

  //Filter Data
  XORSATFilterSerialData xsfsd;
} MPHFSerialData;

uint8_t MPHFSerialize(FILE *pMPHFFile, MPHFQuerier *mphfq);
MPHFQuerier *MPHFDeserialize(FILE *pMPHFFile);

#endif
