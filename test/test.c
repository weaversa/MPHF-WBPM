#include <sys/time.h>
#include <time.h>

#include "mphf.h"

int main() {

  struct timeval tv1;
  struct timezone tzp1;
  gettimeofday(&tv1, &tzp1);
  uint32_t random_seed = ((tv1.tv_sec & 0177) * 1000000) + tv1.tv_usec;

  fprintf(stderr, "random seed = %d\n", random_seed);
  srand(random_seed);

  uint32_t nElements = 1<<20;//12288;
  uint32_t i, j;


  //New

  uint32_t nThreads = 16;
  MPHFBuilder *mphfb = MPHFBuilderAlloc(nElements);

  size_t nElementBytes = 10;
  uint8_t *pElement = malloc(nElementBytes * sizeof(uint8_t));
  if(pElement == NULL) {
    fprintf(stderr, "malloc() failed...exiting\n");
    return -1;
  }

  time_t start_wall = time(NULL);
  clock_t start_cpu = clock();
  
  for(i = 0; i < nElements; i++) {
    for(j = 0; j < nElementBytes; j++) {
      pElement[j] = (uint8_t)(rand()%256);
    }
    if(MPHFBuilderAddElement(mphfb, pElement, nElementBytes) != 0) {
      fprintf(stderr, "Element insertion failed...exiting\n");
      return -1;
    }
  }

  MPHFQuerier *mphfq = MPHFBuilderFinalize(mphfb,
					   //MPHFEfficientParameters,
                                           MPHFPaperParameters,
                                           //MPHFFastParameters,
                                           nThreads);

  clock_t end_cpu = clock();
  time_t end_wall = time(NULL);
  double time_wall = difftime(end_wall, start_wall);
  double time_cpu = ((double) (end_cpu - start_cpu)) / (double) CLOCKS_PER_SEC;

  fprintf(stdout, "\nBuilding took %1.0lf wallclock seconds and %1.0lf CPU seconds\n", time_wall, time_cpu);

  MPHFBuilderFree(mphfb);
  
  if(mphfq == NULL) {
    fprintf(stderr, "MPHF Building Failed\n");
    return -1;
  }
  
  FILE *fout = fopen("test.mphf", "w");
  if(MPHFSerialize(fout, mphfq) != 0) {
    fprintf(stderr, "Serialization failed...exiting\n");
    return -1;
  }
  fclose(fout);
  MPHFQuerierFree(mphfq);

  fout = fopen("test.mphf", "r");
  mphfq = MPHFDeserialize(fout);
  if(mphfq == NULL) {
    fprintf(stderr, "Deserialization failed...exiting\n");
    return -1;
  }
  fclose(fout);

  
  uint8_t *seen = calloc(nElements, sizeof(uint32_t));
  
  srand(random_seed);
  for(i = 0; i < nElements; i++) {
    for(j = 0; j < nElementBytes; j++) {
      pElement[j] = (uint8_t)(rand()%256);
    }

    uint32_t key = MPHFQuery(mphfq, pElement, nElementBytes);
    seen[key] = 1;
  }

  uint32_t failures = 0;
  for(i = 0; i < nElements; i++) {
    if(seen[i] == 0) {
      failures++;
      fprintf(stderr, "Failure at %u\n", i);
    }
  }

  fprintf(stderr, "Passed = %4.2f%%\n", 100.0 * ((double) (nElements - failures)) / (double) nElements);

  free(seen);

  //Efficiency calculation needed.

  fprintf(stdout, "\nTesting query speed with util func: %u queries per second\n", MPHFQueryRate(mphfq));

  uint64_t nMPHFBits = MPHFSize(mphfq);
  
  fprintf(stdout, "Seralized object uses %"PRIu64" total bits, %4.3lf bits per element\n", nMPHFBits, ((double)nMPHFBits) / (double) nElements);
  
  MPHFQuerierFree(mphfq);

  return 0;
}
