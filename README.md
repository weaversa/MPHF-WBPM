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

This project relies on some git submodules. To get these module, clone
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

# More to come