# Introduction

We are Team AVX-512. "We" consists of Avery Crespi, Cole Greer, and Oscar Smith-Sieger. Our project was a journey through the depths of LLVM, icgrep, and the AVX-512 spec. In the end, we learned a lot, discovered some neat things, and gained a disconcerting appreciation for SIMD technology.

### Where It All Began

When we started we had one simple goal:

> To utilize the capabilities of AVX-512 extensions to improve the performance of Parabix applications.

At that point in time icgrep had no specific support for AVX-512. And in fact, many of the operations fell back to incorrectly optimized operations for other instruction sets.

We quickly realized that "improving performance" is no simple task. Icgrep, and not to mention LLVM, is an incredibly complex and interconnected piece of technology. Simply finding where to begin was a challenge for us at the start. But eventually, we did become aquainted with everything and started making headway.

### The Goals

Once things started actually progressing, we set ourselves a number of goals. Some we completed, some became but a pipe dream. More or less, our goals were to find interesting and efficient implementations for the following operations:

 * `hsimd_packh` and `hsimd_packl`
 * `hsimd_signmask`
 * `bitblock_add_with_carry`
 * `bitblock_indexed_advance`
 * `simd_popcount`
 * `simd_pext`
 * `simd_pdep`

We also kept our eye out for any other operations we found along the way which we could improve.


# General Work

So as you can probably imagine, we ended up straying a non-zero amount from our initial plan. Though honestly, a lot less than we'd initially thought.

Most of these "other" things we worked on were about improving the design or architecture related to AVX-512. So while they may not have given a performance boost or anything, we do think they were important and useful contributions to the long-term success of icgrep.

### Dynamic Feature Detection

Probably the largest of these changes is what we're calling Dynamic Feature Detection. It is.. <insert cole stuff here!>

### Overrides To Other IDISA Implementations

The next general thing we worked on was adding specific overrides to the AVX-512 code that pointed to other IDISA implementations. This was because, by default, if we didn't specify an implementation the code fell down to the basic IDISA version. Turns out, those weren't exactly the most efficient implementation in all cases.




# Challenges

### Intrinsic Discovery

### LLVM


# Implementation Details

### hsimd_packh and hsimd_packl

### esimd_popcount

### esimd_bitspread


# Implementation Evaluation


# Conclusion

### Lessons Learned

### Future Work