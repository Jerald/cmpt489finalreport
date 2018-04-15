### `hsimd_packh` and `hsimd_packl`

The operations `hsimd_packh` and `hsimd_packl` take 2 vectors as their input, select the higher or lower half of each element respectively, then concatenates them into a single output vector.

These packing operations are a critical piece of the Parabix Transposition algorithm and contribute significantly to the transposition overhead associated with Parabix applications.

#### Example

Input 1:

```c
[ ABCD, EFGH, IJKL, MNOP ]
```

Input 2:

```c
[ abcd, efgh, ijkl, mnop ]
```

`hsimd_packh` output:

```c
[ AB, EF, IJ, MN, ab, ef, ij, mn ]
```

`hsimd_packl` output:

```c
[ CD, GH, KL, OP, cd, gh, kl, op ]
```

#### Our Implementation

The new vpmov instruction forms the core of our implementation. We used the Intel Intrinsic `mm512_cvtepi16_epi8` which converts packed 16-bit integers in an input to packed 8-bit integers using truncation and then stores the results in an output. This is essentially half of the `packl` operation. Our algorithm for `packl` uses two calls to this intrinsic and then uses a shuffle vector to concatenate the results.

For `packh` we right shifted the input by half the field width then followed the implementation from `packl`.


### Dynamic Feature Detection

The idea behind dynamic feature detection is to move some of the CPU feature detection responsibilities out of `idisa_target.cpp` and instead check available features as needed from within our IR builder class.

This allows us to greatly simplify the inheritance hierarchy for the AVX-512 builder while providing better coverage for future CPUs.

With dynamic feature detection we are able to select the best implementation available for every IDISA function on any given CPU.

For example, if run with a hypothetical CPU which supports AVX512VPOPCNTDQ but does not include AVX512BW, we would be unable to use our improved `hsimd_packh` and `hsimd_packl` functions with a field width of 16 as the given CPU does not support the necessary operations. In this case the `IDISA_AVX512F_Builder` class would fall back to the base `IDISA_Builder` implementations of `hsimd_packh` and `hsimd_packl`. With dynamic feature detection this setback remains isolated to only the functions which require the missing AVX512BW. In this example the IR builder would still be able to take full advantage of the AVX512VPOPCNTDQ capabilities in it's implementation of `simd_popcount`.

While it is very unlikely that intel ever releases a CPU like the one described above, it is useful to demonstrate how flexible and robust the dynamic feature detection is compared to the current fixed inheritance tree.

In the average use case with a "normal" CPU, dynamic feature detection doesn't offer any performance benefits over the current fixed inheritance tree, but it still offers significant benefits with regards to programming style. Only having one version of the `IDISA_AVX512_Builder` class to maintain simplifies the workload for developers. We've also found that it's easier to write code in this style as it provides the programmer with greater flexibility.


### `simd_popcount`

This operation actually corresponds with the reasonably common process just known as popcount. In essence, it returns the number of 1 bits set in a field. Compared to many of the IDISA operations, `simd_popcount` is actually quite rudimentary and popcount even has support for being directly expressed in LLVM IR. For this reason, we were somewhat surprised to find that LLVM was not generating the most efficient code on CPUs without hardware support for vector popcount operations.

#### Our Implementation

With popcount being such a standard operation and particularly due to it's importance in cryptography, lots of research has already gone into efficient implementations without specialized hardware support. We found what we believe to be the fastest known algorithm in the [Hamming weight Wikipedia page](https://en.wikipedia.org/wiki/Hamming_weight#Efficient_implementation). While the implementation documented in the article is for performing popcount on scalar 64 bit values, vectorization is trivial as it only requires bitwise logic and shifts.


### Future Work

#### simd_pext and simd_pdep

##### Current State

We explored the possibility of improving the performance of pext and pdep using the capabilities of AVX512F and AVX512BW as those are the subfamilies which we currently have hardware support for. Unfortunately, we reached the conclusion that we could not make improvements over the default `IDISA_Builder` implementation as AVX512F and AVX512BW lack the capability to address and manipulate individual bits.

##### Future

It remains an open question whether or not future instruction set extensions will unlock new possibilities with pext and pdep. A better implementation might be possible with AVX512VBMI or AVX512VBMI2. It is also possible that better options exist outside of the AVX512 family.

##### u8u16

In it's current state, `u8u16` is not working correctly when run with a blocksize of 512. Currently `u8u16` will give the correct output on the first 512 bits of input it ingests. The output corresponding to the next 512 bits of input gets zeroed out. The remainder of the output stream alternates between blocks of correct output, blocks of zeroes, and on rare occasions blocks of 0xbebe. We have not found a consistent pattern for the order and frequency of these alternations.
