## Introduction

We are Team AVX-512. "We" consists of Avery Crespi, Cole Greer, and Oscar Smith-Sieger. Our project is a

### Where It All Began

When we started we had one simple goal:

> To utilize the capabilities of AVX-512 extensions to improve the performance of Parabix applications.

At that point in time icgrep had no specific support for AVX-512. And in fact, almost all the operations performed falled back to comparatively inefficient base IDISA implementations.


### The Goals


## General Work

### Dynamic Feature Detection


## Challenges

### Intrinsic Discovery

### LLVM


## Implementation Details

### hsimd_packh and hsimd_packl

### esimd_popcount

### esimd_bitspread


## Implementation Evaluation


## Conclusion

### Lessons Learned

### Future Work