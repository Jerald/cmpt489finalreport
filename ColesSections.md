### hsimd_packh and hsimd_packl

##### Overview

hsimd_packh and hsimd_packl are functions which take 2 vectors as input, select the higher or lower half of each element respectively, then concatenates them into a single output vector.

Packing operations are a critical piece of the Parabix Transposition algorithm and contribute significantly to the transposition overhead associated with Parabix applications.

##### Example

Input 1:

``[ ABCD, EFGH, IJKL, MNOP ]``

Input 2:

``[ abcd, efgh, ijkl, mnop ]``

Packh Output:

``[ AB, EF, IJ, MN, ab, ef, ij, mn ]``

Packl Output:

``[ CD, GH, KL, OP, cd, gh, kl, op ]``

##### Our Implementation

The new vpmov instruction forms the core of our implementation. We used intel's 'm256i mm512_cvtepi16_epi8 (m512i a)' which Converts packed 16-bit integers in a to packed 8-bit integers with truncation, and store the results in dst. This is essentially half of packl. Our algorithm for packl uses two calls to this intrinsic and then uses a shuffle vector to concatenate the results.

For packh we right shifted the input by half the field width then followed the implementation from packl.


### Dynamic Feature Detection

The idea behind dynamic feature detection is to move some of the CPU feature detection responsibilities out of the idisa_target class and instead check available features as needed from within the IR builder class.

This allows us to greatly simplify the idisa_builder inheritance hierarchy while providing better coverage for future CPUs.

With dynamic feature detection we are able to select the best implementation available for every idisa function on any given CPU.

For example if run with a hypothetical CPU which supports AVX512VPOPCNTDQ but does not include AVX512BW, we would be unable to use our improved hsimd_packh and hsimd_packl functions with a field width of 16 as the given CPU does not support the necessary operations. In this case the IDISA_AVX512F_Builder would fall back to the base IDISA_Builder implementations of hsimd_packh and hsimd_packl. With dynamic feature detection this setback remains isolated to only the functions which require the missing AVX512BW. In this example the IR builder would still be able to take full advantage of the AVX512VPOPCNTDQ capabilities in it's implementation of simd_popcount.

While it is very unlikely that intel ever releases a CPU like the one described in the example above, it is useful to demonstrate how flexible and robust the dynamic feature detection is over some fixed inheritance tree.

In the average use case with a "normal" CPU, dynamic feature detection doesn't offer any performance benefits over a fixed inheritance tree, it still offers significant benefits with regards to programming style. Only having one version of the idisa_avx512_builder class to maintain simplifies the workload for developers. We've also found that it is easier to write code in this style as it provides the programmer with greater flexibility.


### simd_popcount

##### Overview

Popcount is a simple function which just returns the number of 1 bits are set in a field. Compared to many of the IDISA functions popcount is a very standard operation and can even be directly expressed in llvm IR. For this reason we were somewhat surprised to find that llvm was not generating the most efficient code on CPUs without hardware support for vectorized popcounts.

##### Our Implementation

With popcount being such a standard operation and particularly due to it's importance in cryptography, lots of research has already gone into efficient implementations without specialized hardware support. We found what we believe to be the fastest known algorithm to be documented very well in the [Hamming weight wikipedia page](https://en.wikipedia.org/wiki/Hamming_weight#Efficient_implementation). While the implementation documented in the wikipedia is for performing popcount on scalar 64 bit values, vectorization is trivial as it only uses bitwise logic and shifts.


### Future Work

#### simd_pext and simd_pdep

##### Current State

We explored the possibility of improving the performance of pext and pdep using the capabilities of AVX512F and AVX512BW as those are the subfamilies in which we currently have hardware support. Unfortunately we reached the conclusion that we could not make improvements over the default idisa_builder implementation as AVX512BW lacks the capability to address and manipulate individual bits.

##### Future

It remains an open question whether or not future instruction set extensions will unlock new possibilities with pext and pdep. A better implementation might be possible with AVX512VBMI or AVX512VBMI2. It is also possible that better options exist outside of the AVX512 family.
