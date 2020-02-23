# MPHF-WBPM

Minimal Perfect Hash Functions using weighted bipartite perfect matching and XORSAT.


# Description

This project supports building minimal perfect hash functions
for large sets at roughly 1.83 bits per element.

An MPHF is a bijective function that maps a set of keywords `W = {w_0,
..., w_{n-1}}` to the integers `{0, ..., n-1}`. Details of the
encoding used here are presented in this paper: [Constructing Minimal
Perfect Hash Functions Using SAT
Technology](https://www.cs.cmu.edu/~mheule/publications/AAAI-WeaverS.1625.pdf).


# Dependencies

This project relies on pthreads and some git submodules. To get these, clone
the repository by doing either
```
git clone --recursive git@github.com:weaversa/MPHF-WBPM.git
```
or
```
git clone git@github.com:weaversa/MPHF-WBPM.git
cd MPHF-WBPM
git submodule update --init --recursive
```

# Install

Run `make` in the project root directory. The library file
`libmphfwbpm.a` will (assuming successful compilation) be
created in this package's `lib` directory


# Building an MPFH

An MPHF is built in two separate phases. The first involves adding
elements to a builder object. The second involves creating a querier
object from a builder object. Once completed, the querier object may
be queried ad infinitum.

A builder is first allocated using `MPHFBuilderAlloc`, like so:

```
MPHFBuilder *mphfb = MPHFBuilderAlloc(0);
```

Here, the first and only argument `0` is the number of expected
elements the MPHF will encode. It is safe to leave this number as `0`,
but will decrease calls to malloc if the actual number is given ahead
of time.

Elements are added to the builder, like so:

```
if(MPHFBuilderAddElement(mphfb, pElement, nElementBytes) != 0) {
  fprintf(stderr, "Element insertion failed...exiting\n");
  return -1;
}
```

Here, `pElement` is a pointer to at least `nElementBytes` number of
bytes. This element will be copied into the builder.

After all elements have been stored, the querier is ready to be
created:

```
MPHFQuerier *mphfq = MPHFBuilderFinalize(mphfb, MPHFPaperParameters, nThreads);
```

The first argument is the builder. The second argument is a structure
consisting of two parameters: the number of elements per block (the initial split to parallelize over) and the XORSAT filter parameters.
For example,

```
MPHFParameters MPHFPaperParameters =
  { .nEltsPerBlock = 12288,
    .xsfp =
    { .nLitsPerRow   = 0,
      .nSolutions    = 1,
      .nEltsPerBlock = 4608,
      .fEfficiency   = 1.00 },
  };
```

Feel free to define your own parameters to meet the needs of your
application.

The returned querier (`mphfq`) will be `NULL` on error.

When finalizing, you will notice that some progress is printed to
stderr. These print statements can be turned off by commenting out the
following line in mphf.h and recompiling the package.

```
#define MPHF_PRINT_BUILD_PROCESS
```

After creating the querier, it is suggested that the builder be
free'd, like so:

```
MPHFBuilderFree(mphfb);
```


# Querying an MPFH

The MPHF can be queried against an element, like so:

```
uint32_t key = MPHFQuery(mphfq, pElement, nElementBytes);
```

Here, `pElement` is a pointer to `nElementBytes` number of bytes. The
key unique to this element is returned.

When querying is finished, the querier can be freed, like so:

```
  MPHFQuerierFree(mphfq);
```


# Serialization

Queriers can be serialized (written to a file) in the following way:

```
  FILE *fout = fopen("mphf.out", "w");
  if(MPHFSerialize(fout, mphfq) != 0) {
    fprintf(stderr, "Serialization failed...exiting\n");
    return -1;
  }
  fclose(fout);
```

Here, `fout` is of type `FILE *`. `ret` will be `0` on failure and `1`
on success.

A querier can be deserialized (read from a file) in the following way:

```
  fout = fopen("mphf.out", "r");
  mphfq = MPHFDeserialize(fout);
  if(mphfq == NULL) {
    fprintf(stderr, "Deserialization failed...exiting\n");
    return -1;
  }
  fclose(fout);
```

Here, `fout` is of type `FILE *`. `mphfq` will be `NULL` on error.


# Linking

To use, simply link against `lib/libmphfwbpm.a` and include
`include/mphf.h`.


# Test

A sample interface is given in the `test` directory. The test builds
an MPHF for 2^20 random 10-byte elements and then queries the MPHF
against the original elements (for a consistency check) and prints
statistics. To run the test type:

```
$ make test/test && test/test
```


# Further Information

A paper about MPHF and WBPM is available here: [Constructing Minimal
Perfect Hash Functions Using SAT
Technology](https://www.cs.cmu.edu/~mheule/publications/AAAI-WeaverS.1625.pdf).
