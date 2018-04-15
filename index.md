# Introduction

We are Team 512. "We" consists of Avery Crespi, Cole Greer, and Oscar Smith-Sieger. Our project was a journey through the depths of LLVM, icgrep, and the AVX-512 spec. In the end, we learned a lot, discovered some neat things, and gained a disconcerting appreciation for SIMD technology.

### Where It All Began

When we started we had one simple goal:

> To utilize the capabilities of AVX-512 extensions to improve the performance of Parabix applications.

At that point in time icgrep had no specific support for AVX-512. And in fact, many of the operations fell back to incorrectly optimized operations designed for other instruction sets.

We quickly realized that "improving performance" is no simple task. Icgrep, and not to mention LLVM, is an incredibly complex and interconnected piece of technology. Simply finding where to begin was a challenge for us at the start. But eventually, we did become aquainted with everything and started to make some headway.

### The Real Goals

Once things actually started progressing, we set ourselves a number of goals. Some we completed, some became but a pipe dream. But more or less, our goals were to find interesting and efficient implementations for the following operations:

 * `hsimd_packh` and `hsimd_packl`
 * `hsimd_signmask`
 * `bitblock_add_with_carry`
 * `bitblock_indexed_advance`
 * `simd_popcount`
 * `simd_pext`
 * `simd_pdep`

Though we were sure to keep our eyes out for any other operations we found along the way which could be improved.

### Our Tools Of The Trade

In a project such as ours, one based around improving the execution of algorithms using newly available tools, we had two directions to head. The first was to rework and reengineer the existing algorithms in ways that facilitated and utilized the new AVX-512 features. The second was to analyze the existing processes and look for cases where they can be improved by direct application of new AVX-512 tools. The latter, honestly far easier path, was the one we chose.

This meant that the primary means by which we were looking to improve things was through discovering new and interesting LLVM Intrinsics which could be applied in unique ways. This of course was heavily facilitated by the existence of the Intel Intrinsics Guide, albeit it was not quite as useful as we originally would've hoped. Through our adventures, we found LLVM and its Intrinsics were a whole different beast than the Intel ones we had information on. But that's a story for another section; in due time all will be revealed.

---

# General Work

So as you can probably imagine, we ended up straying a non-zero amount from that initial plan. Though honestly, we strayed a lot less than we'd initially thought.

Most of these "other" things we worked on were about improving the design or architecture related to AVX-512. So while they may not have given a performance boost or anything too tangible, we do think they were important and useful contributions to the long-term success of icgrep.

### Dynamic Feature Detection

Probably the largest of these changes is what we're calling dynamic feature detection. The idea behind this change is to move some of the CPU feature detection responsibilities out of `idisa_target.cpp` and instead check available features as needed from within our IR builder class.

This allows us to greatly simplify the inheritance hierarchy for the AVX-512 builder while providing better coverage for future CPUs.

With dynamic feature detection we are able to select the best implementation available for every IDISA function on any given CPU.

For example, if run with a hypothetical CPU which supports AVX512VPOPCNTDQ but does not include AVX512BW, we would be unable to use our improved `hsimd_packh` and `hsimd_packl` functions with a field width of 16 as the given CPU does not support the necessary operations. In this case the `IDISA_AVX512F_Builder` class would fall back to the base `IDISA_Builder` implementations of `hsimd_packh` and `hsimd_packl`. With dynamic feature detection this setback remains isolated to only the functions which require the missing AVX512BW. In this example the IR builder would still be able to take full advantage of the AVX512VPOPCNTDQ capabilities in it's implementation of `simd_popcount`.

While it is very unlikely that intel ever releases a CPU like the one described above, it is useful to demonstrate how flexible and robust the dynamic feature detection is compared to the current fixed inheritance tree.

In the average use case with a "normal" CPU, dynamic feature detection doesn't offer any performance benefits over the current fixed inheritance tree, but it still offers significant benefits with regards to programming style. Only having one version of the `IDISA_AVX512_Builder` class to maintain simplifies the workload for developers. We've also found that it's easier to write code in this style as it provides the programmer with greater flexibility.

---

### Overrides To Other IDISA Implementations

The next general thing we worked on was adding specific overrides to the AVX-512 code that pointed to other IDISA implementations. This was because, by default, if we didn't specify an implementation the code fell down to the basic IDISA version. Turns out, those weren't exactly the most efficient implementation in all cases.

#### What We Did

At the beginning, just after AVX-512 support was added, our builder was inheriting only from the base IDISA builder. So our first step was to instead make it inherit from the AVX2 builder. But that caused the issue of the builder descending the inheritance tree to find the implementation to use. At first glance that may seem good, but the issue arose that many of the previous functions had been optimized for other block sizes and field widths. This caused a few cases where performance was actually *worse* then when we started. Notably, the AVX2 `hsimd_signmask` will fall through to the SSE implementation which contains no guards for field width or bitblock width. This causes a notable slow-down versus the base IDISA implementation.

From there, we went and found every function overridden in a previous builder, other than the base IDISA builder, and overrode it in our builder. We were then able to precisely specify which version of a given operation we wanted to be using.

#### How This Helped

The biggest improvement by far from this change was inheriting the SSE2 implementation of `bitblock_advance`. This version has been optimized quite well, and offered a nearly 30% runtime improvement for large files.

---

# Challenges

Of course, in a project like this there were bound to be some challenges. But we didn't predict quite this many challenges arising. Right at the beginning there was the obvious challenge of becoming acquainted with the codebase, but that was just the beginning of our journey.

### Intrinsic Locating and Usage

One of the first real issues that arose was a pretty simple one: how do we *use* an intrinsic? We had found a useful intrinsic through the Intel Intrinsics Guide, but we had no clue how to use it in LLVM. The x86 assembly mnemonic didn't seem to be of much use, and the Intel Intrinsic name was just as meaningless to LLVM. That's when the digging began.

#### The First Breakthrough

So we had previously known that you can use the Intel Intrinsics in your code through adding the header `immintrin.h`. What we didn't know was how the intrinsics were defined in said header. Turns out, they're not defined as assembly black magic as we'd previously thought, they're implemented in terms of these weird things called GCC Builtins. A GCC Builtin is more or less the closest analogue to an LLVM Intrinsic. Basically, it's some arbitrary operation that's defined internally by GCC.

Now this may not seem like it was that useful, and initially, it wasn't. But one time, while wallowing in our failure, we got annoyed and just tried searching through the entire icgrep source directory for an intrinsic. We first did it with the assembly mnemonic, which turned up a mess of scariness we never touched again. But then we tried it again with that GCC Builtin we'd found. That's where things started to turn around.

#### The Second Finding

By searching through the icgrep directory like that we found a number of things, most of them useless. But there were two lines which were of interest to us:

```cpp
return Intrinsic::x86_avx512_mask_pmov_wb_512;    // "__builtin_ia32_pmovwb512_mask"
GCCBuiltin<"__builtin_ia32_pmovwb512_mask">,
```

The comment on that first line was in fact the Builtin we were searching for. Given the context, we could only assume the thing called "Intrinsic" was probably the LLVM Intrinsic we were looking for. As it turns out, it was! With this we now had a semi-reliable, albeit tedious, method to find the LLVM Intrinsic that corresponds to an Intel Intrinsic.

#### The Third Discovery

But now that we have the name to call, how do we actually, well, *use* it in code? We don't really know the parameters to pass it, only some vague hint from the Intel Intrinsics Guide. Well our first idea was to look back at the intrinsic implementation we found in `immintrin.h`. It turns out that along with the Builtin, there is some code actually calling it. That provided us a vague hint at the very least. We decided that since the LLVM Intrinsics are based off the GCC Builtins, it made sense if they were called the same. And what did you know, it actually worked! We had code that ran and performed the operations we wanted. Except, as we soon realized, everything was not as it seemed.

#### The Fourth Revelation

Turns out, LLVM often wants different, and more specific, types for the parameters than GCC does. This was a hard problem to solve, because as of then we hadn't found any part of this actually in LLVM. Well, our saviour came in the form of that second line from above:

```cpp
GCCBuiltin<"__builtin_ia32_pmovwb512_mask">,
```

The file this came from is called a tablegen file. It's part of how LLVM generates targets, basically a backend it's compiling for. When we looked at the file the line came from, we found this:

```cpp
def int_x86_avx512_mask_pmov_wb_512 :
          GCCBuiltin<"__builtin_ia32_pmovwb512_mask">,
          Intrinsic<[llvm_v32i8_ty],
                    [llvm_v32i16_ty, llvm_v32i8_ty, llvm_i32_ty],
                    [IntrNoMem]>;
```

This of course was basically complete gibberish to us. All we could gather was that it defined something which looked basically like our intrinsic, and it was somehow connecting that to the GCC Builtin. Well after a long and painful investigation we don't have the time to get into, we eventually found out how to read these tablegen entries.

As it turns out, within `Intrinsic<>`, the first part in brackets is what the intrinsic returns and the second part in brackets is the *ordered list* of parameters. That last part was the most important. With that we officially knew the types, and even the correct order of them, needed by the intrinsic.

#### The Conclusion

Using everything we found out, we, in true compsci fashion, created a script to do all this hard work for us. Simply provided with an Intel Intrinsic, it will tell us the GCC Builtin, LLVM Intrinsic, source header, `immintrin.h` definition, and even the LLVM usage. Although being completely unrelated to any actual improvements in icgrep or Parabix, this was actually one of the most useful breakthroughs in our opinion. Because of this, we've included a cleaned up and optimized version of our script in appendix <INSERT APENDIX NUMBER HERE!>. We hope it can perhaps offer as much help, and save as much pain, for others as it has for us.

---

### LLVM

With all the talk about how much trouble we had with LLVM related things, you might think we've already covered our troubles with LLVM. I wish that were true, but this is only the beginning. We don't call it HeLLVM for nothing.

#### Being Selected

A core part of LLVM's backend compilation phase is turning its IR into a sort of internal pseudo-assembly it uses to "select" the actually assembly instruction for a given backend. Relevant to us, this is the process which takes a given LLVM Intrinsic and correctly translates it to the relevant x86 assembly mnemonic. At least that's what it's supposed to do. We ran into a number of issues where LLVM returned an error saying it "couldn't select" an assembly instruction from one of our Intrinsics.

#### Narrowing Down The Problem

Our first thought was that we had somehow found an incorrect intrinsic, or were perhaps using it wrong. This took us delving deep into the LLVM source to look for the problem. What we found was that the Intrinsic did in fact exist, and we were in fact using it right. So what were we doing wrong? Unfortunately, the answer was *nothing*.

#### Getting To The Bottom Of It

It turns out that when LLVM said it "couldn't select" an instruction, it wasn't lying. It seems that the version of LLVM used in icgrep simply doesn't implement the ability to use the instruction we wanted. LLVM literally couldn't generate the right assembly because, as far as it was aware, the instruction didn't exist. We had hoped that when we finally got to the bottom of this issue, we would find some way to remedy it. But sadly, this is a problem we have no way to fix and can only hope has been found in modern versions of LLVM.

---

# Implementation Details

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

---

### `simd_popcount`

This operation actually corresponds with the reasonably common process just known as popcount. In essence, it returns the number of 1 bits set in a field. Compared to many of the IDISA operations, `simd_popcount` is actually quite rudimentary and popcount even has support for being directly expressed in LLVM IR. For this reason, we were somewhat surprised to find that LLVM was not generating the most efficient code on CPUs without hardware support for vector popcount operations.

#### Our Implementation

With popcount being such a standard operation and particularly due to it's importance in cryptography, lots of research has already gone into efficient implementations without specialized hardware support. We found what we believe to be the fastest known algorithm in the [Hamming weight Wikipedia page](https://en.wikipedia.org/wiki/Hamming_weight#Efficient_implementation). While the implementation documented in the article is for performing popcount on scalar 64 bit values, vectorization is trivial as it only requires bitwise logic and shifts.

---

### `bitblock_add_with_carry`

The purpose of `bitblock_add_with_carry` is long-stream addition (with carries) of 64-bit values. This form of addition is not natively supported by AVX2 or AVX-512BW intrinsics, so an efficient implementation is important. Our changes provide performance improvements without altering the existing algorithm.

#### AVX2 Implementation

The AVX2 implementation of `bitblock_add_with_carry` uses many simple SIMD operations, which we found to be well-optimized by the LLVM backend. It also calls the `esimd_bitspread` function, which is implemented with generic SIMD operations.

```cpp
std::pair<Value *, Value *> IDISA_AVX2_Builder::bitblock_add_with_carry(Value * e1, Value * e2, Value * carryin) {
    // Code omitted for brevity ...
    Value * increments = esimd_bitspread(64,incrementMask);
    // Code omitted for brevity ...
}
```

We focused on implementing `esimd_bitspread` with AVX-512F intrinsics to improve performance.

#### Overview of `esimd_bitspread`

The purpose of this operation is to spread each bit from a bitmask into a full-width field. Our new implementation uses AVX-512F broadcast intrinsics to improve performance.

#### Generic SIMD Implementation of `esimd_bitspread`

The generic SIMD implementation of `esimd_bitspread` uses `ShuffleVector` operations and multiple other vectors and bitcasts. These LLVM operations do not always generate efficient IR (especially `ShuffleVector`), so by avoiding them we can significantly improve performance.

```cpp
Value * IDISA_Builder::esimd_bitspread(unsigned fw, Value * bitmask) {
    // Code omitted for brevity ...
    Value * spread_field = CreateBitCast(CreateZExtOrTrunc(bitmask, field_type), VectorType::get(getIntNTy(fw), 1));
    Value * undefVec = UndefValue::get(VectorType::get(getIntNTy(fw), 1));
    Value * broadcast = CreateShuffleVector(spread_field, undefVec, Constant::getNullValue(VectorType::get(getInt32Ty(), field_count)));
    // Code omitted for brevity ...
}
```

#### AVX-512F Implementation of `esimd_bitspread`

`bitblock_add_with_carry` always calls `esimd_bitspread` with a field width of 64. If the field width is not 64, we fall back to the generic SIMD implementation of `esimd_bitspread`.

Our implementation broadcasts `i64` values to the destination vector using the given 8-bit mask. For every one bit in the mask, an `i64` 1 will be copied from the `a` vector. For every zero bit in the mask, an `i64` 0 will be copied from the fallback `src` vector. The resulting `<8 x i64>` will contain 8 `i64` values corresponding to the 8 bits in the mask.

```cpp
llvm::Value * IDISA_AVX512BW_Builder::esimd_bitspread(unsigned fw, Value * bitmask) {
    if (mBitBlockWidth == 512 && fw == 64) {
        Value * broadcastFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::x86_avx512_mask_broadcasti64x4_512);

        const unsigned int srcFieldCount = 8;
        Constant * srcArr[srcFieldCount];
        for (unsigned int i = 0; i < srcFieldCount; i++) {
            srcArr[i] = getInt64(0);
        }
        Constant * src = ConstantVector::get({srcArr, srcFieldCount});

        const unsigned int aFieldCount = 4;
        Constant * aArr[aFieldCount];
        for (unsigned int i = 0; i < aFieldCount; i++) {
            aArr[i] = getInt64(1);
        }
        Constant * a = ConstantVector::get({aArr, aFieldCount});

        Value * broadcastMask = CreateZExtOrTrunc(bitmask, getInt8Ty());

        return CreateCall(broadcastFunc, {a, src, bitmask});
    }
    return IDISA_Builder::esimd_bitspread(fw, bitmask);
}
```

---

# Implementation Evaluation

As the overarching goal of the project is to improve performance, the best way we found to evaluate our code is by benchmarking the runtime of icgrep. To this end, we've carried out a number of tests to see the benefits gained by our work.

### Methodology

All tests were performed on the CSIL AVX-512 server `cs-osl-08`.

Every test was run 10 times in succession, and the best result (shortest execution time) was taken.

### Test Details

We tested with a complex regular expression on a large input file in order to best represent the normal use case of icgrep.

```cpp
Regex: [A-Z]((([a-zA-Z]*a[a-zA-Z]*[ ])*[a-zA-Z]*e[a-zA-Z]*[ ])*[a-zA-Z]*s[a-zA-Z]*[ ])*[.?!]
```

```cpp
Input file: /home/cameron/Wikimedia/dewiki-20150125-pages-articles.xml (14 GB Unicode text)
```

We measured performance with the command `perf stat icgrep -c -r $regex $input -BlockSize=$size`, where `$regex` is the regex, `$input` is the input file, and `$size` is the block size.

### Test Results

"Original" icgrep refers to icgrep rev. 5968 without our changes (`packh` and `packl` reverted).

"Modified" icgrep refers to icgrep rev. 5968 merged with our changes.

#### Original vs. modified with BlockSize=256

To establish a baseline, we first tested with a block size of 256. This test excludes our performance improvements, so we expected performance to be roughly the same.

```cpp
Original:

      19173.678393      task-clock (msec)         #    0.993 CPUs utilized
             3,751      context-switches          #    0.196 K/sec
                 4      cpu-migrations            #    0.000 K/sec
           348,629      page-faults               #    0.018 M/sec
    47,768,061,667      cycles                    #    2.491 GHz
   <not supported>      stalled-cycles-frontend
   <not supported>      stalled-cycles-backend
    94,164,738,864      instructions              #    1.97  insns per cycle
     2,809,844,564      branches                  #  146.547 M/sec
        83,988,313      branch-misses             #    2.99% of all branches

      19.302744652 seconds time elapsed

Modified:

      19303.098007      task-clock (msec)         #    0.994 CPUs utilized
             2,881      context-switches          #    0.149 K/sec
                11      cpu-migrations            #    0.001 K/sec
           348,036      page-faults               #    0.018 M/sec
    48,031,626,212      cycles                    #    2.488 GHz
   <not supported>      stalled-cycles-frontend
   <not supported>      stalled-cycles-backend
    94,403,894,196      instructions              #    1.97  insns per cycle
     2,854,466,925      branches                  #  147.876 M/sec
        83,950,582      branch-misses             #    2.94% of all branches

      19.416177422 seconds time elapsed
```

As we expected, performance was nearly identical across all metrics. Elapsed time was only different by 100ms (~0.6%), which can be accounted for by test variance.

#### Original vs. modified with BlockSize=512

To determine the extent of our performance improvements, we tested with a block size of 512.
We expected the modified icgrep to be significantly faster than the original.

```cpp
Original:

      20962.526286      task-clock (msec)         #    0.998 CPUs utilized
               866      context-switches          #    0.041 K/sec
                 4      cpu-migrations            #    0.000 K/sec
           282,026      page-faults               #    0.013 M/sec
    52,399,033,725      cycles                    #    2.500 GHz
   <not supported>      stalled-cycles-frontend
   <not supported>      stalled-cycles-backend
    76,707,180,495      instructions              #    1.46  insns per cycle
     2,426,347,899      branches                  #  115.747 M/sec
        50,667,782      branch-misses             #    2.09% of all branches

      20.999098691 seconds time elapsed

Modified:

      15290.962991      task-clock (msec)         #    0.948 CPUs utilized
            26,033      context-switches          #    0.002 M/sec
                17      cpu-migrations            #    0.001 K/sec
           346,907      page-faults               #    0.023 M/sec
    37,395,579,783      cycles                    #    2.446 GHz
   <not supported>      stalled-cycles-frontend
   <not supported>      stalled-cycles-backend
    58,805,734,674      instructions              #    1.57  insns per cycle
     2,447,342,368      branches                  #  160.052 M/sec
        50,533,018      branch-misses             #    2.06% of all branches

      16.129089854 seconds time elapsed
```

As we expected, our modified code was 4.9 seconds (23.2%) faster when using a block size of 512.

Our modified icgrep used 15 million (28.6%) fewer cycles and 18 million (23.3%) fewer instructions.
The number of instructions per cycle increased by 0.11 (7.5%), and throughput increased by 44 MB/s (38.3%).

Our modified icgrep also caused significantly more context switches, but this did not appear to impact performance.

### Final Outcome

The performance of icgrep is significantly improved on CPUs that support AVX-512.

The increased blocksize and optimized hardware of CPUs with AVX-512 offer significant performance improvements for parallelized applications such as icgrep.

In addition to these base advantages, our changes to icgrep leverage new AVX-512 intrinsics to further increase throughput and reduce run time.

With icgrep unchanged, `BlockSize=512` tested as 8.8% slower than `BlockSize=256`.

With our changes, `BlockSize=512` tested as 16.9% faster than `BlockSize=256`.

---

# Conclusion

Well, it's been a long, difficult, and interesting road for us. But in the end, we think everything went pretty well. We made considerable improvements to the performance of icgrep on machines with AVX-512, we discovered a number of traps in LLVM which can hopefully be avoided in the future, and we created a new tool for finding intrinsic information. We hope that the work we did helps future icgrep developers on their journey.


### Lessons Learned

Throughout this project we had our ups, our downs, and our side-ways. We learned far more about far weirder things than we would've ever immagined. It's hard for us to come up with a comprehensive list, so here are a few of the ones we thought were most important.

##### The Basics

When we started this course we had zero experience with any relevant software or development techniques. We were only vaguely familiar with SIMD technology from having it off-handedly mentioned when we took CMPT 295. A lot of what we did was simply experimenting with and discovering new things about, LLVM and the Parabix framework. Through this process, we actually became reasonably competant with developing for icgrep and LLVM.

##### For problem solving:

You don't necessarily need to find vector-specific solutions to problems. In some cases, as with popcount, you can find a scalar solution which can be applied easily to vectorized cases.

##### If a solution isn't working:

Sometimes a solution which uses less IR and, should at least, use less assembly can still be worse than an existing solution. This was something that came up a few times, notably with `bitblock_advance` and also somewhat in the case of our unselectable intrinsic.

##### And most importantly:

When we started, we were worried we weren't experienced enough for this course. With us being only second year's, we were worried we wouldn't have the skills to do well. But in the end, the lesson we took from this experience is that if you're offered the opportunity to push the boundries of what you can do, maybe give it a thought.

---

### Future Work

Although we have made a lot of progress, there is far more to be done using AVX-512. We've done preliminary research and/or investigation into a number of promising improvements which we think will benefit icgrep in the long run. There are also a number of operations we've already looked into and evaluated, so hopefully someone else doesn't have to repeat that work.

##### `simd_pext` and `simd_pdep`

We explored the possibility of improving the performance of `pext` and `pdep` using the capabilities of AVX512F and AVX512BW as those are the subfamilies which we currently have hardware support for. Unfortunately, we reached the conclusion that we could not make improvements over the default `IDISA_Builder` implementation as AVX512F and AVX512BW lack the capability to address and manipulate individual bits.

It remains an open question whether or not future instruction set extensions will unlock new possibilities with pext and pdep. A better implementation might be possible with AVX512VBMI or AVX512VBMI2. It is also possible that better options exist outside of the AVX512 family.

##### Shuffle Vectors

Coming with AVX-512 are a number of new permutation operations. These provide the ability to convert a number of specific shuffle vector operations into exact intrinsics. The specific family are the `permutex2var` set. There are a number of different Intel Intrinsics matching this for a number of different field widths. These can be used to permute from two inputs, similar to how the LLVM IR `shufflevector` instruction works.

Implementing some sort of override for using these has the possibility to improve a number of different operations in specific cases.

##### Arbitrary Bit Shift

One of the limitations in AVX-512 is the lack of bit-specific or bit-granular operations. This can make a lot of attempts at optimization difficult because IDISA makes use of many bit-level operation. Utilizing multiple new AVX-512 instructions has the possibility to allow for arbitrary shifting at a bit-specific level.

The proposed method we have uses some of the new bit rotate operations, such as `_mm512_rol_epi64`, which rotate the bits in a field such that shifting off one edge has them come around to the other. That can then be used with the 8-bit align operations from `_mm512_alignr_epi8`, which combine 128-bit lanes from two input vectors into a 256-bit temporary value, shifts the value by a supplied number of bytes, and then stores the lower 128-bits into the destination.

These two operations combined have the ability to allow selecting and shifting of an arbitrary bit amount. Unfortunately, because the align operations work on 128-bit lanes or larger, and the rotates only work on 64-bit quads or smaller, you can only effectively work on every second quad. This means that for a 512-bit vector register, you can only process an effective 256-bits of data.

##### u8u16

In it's current state, `u8u16` is not working correctly when run with a blocksize of 512. Currently `u8u16` will give the correct output on the first 512 bits of input it ingests. The output corresponding to the next 512 bits of input gets zeroed out. The remainder of the output stream alternates between blocks of correct output, blocks of zeroes, and on rare occasions blocks of 0xbebe. We have not found a consistent pattern for the order and frequency of these alternations. Unfortunately we ran out of time before we were able to resolve this issue.
