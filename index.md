# Introduction

We are Team AVX-512. "We" consists of Avery Crespi, Cole Greer, and Oscar Smith-Sieger. Our project was a journey through the depths of LLVM, icgrep, and the AVX-512 spec. In the end, we learned a lot, discovered some neat things, and gained a disconcerting appreciation for SIMD technology.

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


# General Work

So as you can probably imagine, we ended up straying a non-zero amount from that initial plan. Though honestly, we strayed a lot less than we'd initially thought.

Most of these "other" things we worked on were about improving the design or architecture related to AVX-512. So while they may not have given a performance boost or anything too tangible, we do think they were important and useful contributions to the long-term success of icgrep.

### Dynamic Feature Detection

Probably the largest of these changes is what we're calling Dynamic Feature Detection. It is.. <insert cole stuff here!>

### Overrides To Other IDISA Implementations

The next general thing we worked on was adding specific overrides to the AVX-512 code that pointed to other IDISA implementations. This was because, by default, if we didn't specify an implementation the code fell down to the basic IDISA version. Turns out, those weren't exactly the most efficient implementation in all cases.


#### What we did

At the beginning, just after AVX-512 support was added, our builder was inheriting only from the base IDISA builder. So our first step was to instead make it inherit from the AVX2 builder. But that caused the issue of the builder descending the inheritance tree to find the implementation to use. At first glance that may seem good, but the issue arose that many of the previous functions had been optimized for other block sizes and field widths. This caused a few cases where performance was actually *worse* then when we started. Notably, the AVX2 `bitblock_add_with_carry` isn't gated by a field width or bitblock width check and has a number of hardcoded values that, while good for AVX2, weren't optimal for AVX-512.

From there, we went and found every function overridden in a previous builder, other than the base IDISA builder, and overrode it in our builder. We were then able to precisely specify which version of a given operation we wanted to be using.

#### How this helped

The biggest improvement by far from this change was inheriting the SSE2 implementation of `bitblock_advance`. This version has been optimized quite well, and offered a nearly 30% runtime improvement for large files.


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

### LLVM

With all the talk about how much trouble we had with LLVM related things, you might think we've already covered our troubles with LLVM. I wish that were true, but this is only the beginning. We don't call it HeLLVM for nothing.




# Implementation Details

### hsimd_packh and hsimd_packl

### esimd_popcount

### `bitblock_add_with_carry`

#### Overview of `bitblock_add_with_carry`

The purpose of the `bitblock_add_with_carry` function is long-stream addition (with carries) of 64-bit values.
This form of addition is not natively supported by AVX2 or AVX-512BW intrinsics, so an efficient implementation is important.
Our changes provide performance improvements without altering the existing algorithm.

#### AVX2 implementation of `bitblock_add_with_carry`

The AVX2 implementation of `bitblock_add_with_carry` uses many simple SIMD operations, which we found to be well-optimized by the LLVM backend.
`bitblock_add_with_carry` also calls the `esimd_bitspread` function, which is implemented with generic SIMD operations.

```
std::pair<Value *, Value *> IDISA_AVX2_Builder::bitblock_add_with_carry(Value * e1, Value * e2, Value * carryin) {
    // Code omitted for brevity ...
    Value * increments = esimd_bitspread(64,incrementMask);
    // Code omitted for brevity ...
}
```

We focused on implementing `esimd_bitspread` with AVX-512BW intrinsics to improve performance.

#### Overview of `esimd_bitspread`

The purpose of `esimd_bitspread` is to spread each bit from a bitmask into a full-width field.
Our new implementation uses AVX-512BW broadcast intrinsics to improve performance.

#### Generic SIMD implementation of `esimd_bitspread`

The generic SIMD implementation of `esimd_bitspread` uses `ShuffleVector`s and multiple other vectors and bitcasts.
These LLVM operations do not always generate efficient IR (especially `ShuffleVector`), so by avoiding them we can significantly improve performance.

```
Value * IDISA_Builder::esimd_bitspread(unsigned fw, Value * bitmask) {
    // Code omitted for brevity ...
    Value * spread_field = CreateBitCast(CreateZExtOrTrunc(bitmask, field_type), VectorType::get(getIntNTy(fw), 1));
    Value * undefVec = UndefValue::get(VectorType::get(getIntNTy(fw), 1));
    Value * broadcast = CreateShuffleVector(spread_field, undefVec, Constant::getNullValue(VectorType::get(getInt32Ty(), field_count)));
    // Code omitted for brevity ...
}
```

#### AVX-512BW implementation of `esimd_bitspread`

`bitblock_add_with_carry` always calls `esimd_bitspread` with a field width of 64.
If the field width is not 64, we fall back to the generic SIMD implementation of `esimd_bitspread`.

Our implementation broadcasts `i64`s to the destination vector using the given 8-bit mask.
For every one bit in the mask, an `i64` 1 will be copied from the `a` vector.
For every zero bit in the mask, an `i64` 0 will be copied from the fallback `src` vector.
The resulting `<8 x i64>` will contain 8 `i64`s corresponding to the 8 bits in the mask.

```
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

# Implementation Evaluation

### Performance

#### Test environment

All tests were performed on the CSIL AVX-512 server `cs-osl-08`.

Every test was run 10 times in succession, and the best result (shortest execution time) was taken.

#### Test details

We tested with a complex regular expression on a large input file in order to best represent the use case of icgrep.

Regex: `[A-Z]((([a-zA-Z]*a[a-zA-Z]*[ ])*[a-zA-Z]*e[a-zA-Z]*[ ])*[a-zA-Z]*s[a-zA-Z]*[ ])*[.?!]`

Input file: `/home/cameron/Wikimedia/dewiki-20150125-pages-articles.xml` (14 GB Unicode text)

We measured performance with the command `perf stat icgrep -c -r $regex $input -BlockSize=$size`, where `$regex` is the regex, `$input` is the input file, and `$size` is the block size.

#### Test results

"Original" icgrep refers to icgrep rev. 5968 without our changes.

"Modified" icgrep refers to icgrep rev. 5968 merged with our changes.

##### Original vs. modified with BlockSize=256

To establish a baseline, we first tested with a block size of 256.
This test excludes our performance improvements, so we expected performance to be roughly the same.

```
Original:

      18932.729419      task-clock (msec)         #    0.994 CPUs utilized
             3,530      context-switches          #    0.186 K/sec
                 2      cpu-migrations            #    0.000 K/sec
           288,333      page-faults               #    0.015 M/sec
    47,192,334,013      cycles                    #    2.493 GHz
   <not supported>      stalled-cycles-frontend
   <not supported>      stalled-cycles-backend
    92,805,993,775      instructions              #    1.97  insns per cycle
     2,591,933,399      branches                  #  136.902 M/sec
        82,445,498      branch-misses             #    3.18% of all branches

      19.039420752 seconds time elapsed

Modified:

      18891.225704      task-clock (msec)         #    0.994 CPUs utilized
             3,111      context-switches          #    0.165 K/sec
                 1      cpu-migrations            #    0.000 K/sec
           293,365      page-faults               #    0.016 M/sec
    47,126,066,272      cycles                    #    2.495 GHz
   <not supported>      stalled-cycles-frontend
   <not supported>      stalled-cycles-backend
    92,796,969,146      instructions              #    1.97  insns per cycle
     2,586,662,681      branches                  #  136.924 M/sec
        83,078,587      branch-misses             #    3.21% of all branches

      19.003514972 seconds time elapsed
```

As we expected, performance was nearly identical across all metrics.
Elapsed time was only different by 30ms (~1%), which can be accounted for by test variance.

##### Original vs. modified with BlockSize=512

To determine the extent of our performance improvements, we tested with a block size of 512.
We expected the modified icgrep to be significantly faster than the original.

```
Original:

      17541.104082      task-clock (msec)         #    0.989 CPUs utilized
             6,739      context-switches          #    0.384 K/sec
                12      cpu-migrations            #    0.001 K/sec
           320,005      page-faults               #    0.018 M/sec
    43,531,265,728      cycles                    #    2.482 GHz
   <not supported>      stalled-cycles-frontend
   <not supported>      stalled-cycles-backend
    64,057,204,343      instructions              #    1.47  insns per cycle
     2,176,657,041      branches                  #  124.089 M/sec
        50,176,285      branch-misses             #    2.31% of all branches

      17.737673631 seconds time elapsed

Modified:

      14829.318856      task-clock (msec)         #    0.954 CPUs utilized
            20,592      context-switches          #    0.001 M/sec
                11      cpu-migrations            #    0.001 K/sec
           320,383      page-faults               #    0.022 M/sec
    36,465,102,857      cycles                    #    2.459 GHz
   <not supported>      stalled-cycles-frontend
   <not supported>      stalled-cycles-backend
    57,415,596,361      instructions              #    1.57  insns per cycle
     2,222,518,780      branches                  #  149.873 M/sec
        49,266,758      branch-misses             #    2.22% of all branches

      15.541295089 seconds time elapsed
```

As we expected, our modified code was 2.2s (12.4%) faster when using a block size of 512.

Our modified icgrep used 7 million (16%) fewer cycles and 7 million (10%) fewer instructions.
The number of instructions per cycle increased by 0.1 (7%), and throughput increased by 25 MB/s (21%).

Our modified icgrep also caused significantly more context switches, but this did not appear to impact performance.

#### Conclusion

The performance of icgrep is significantly improved on CPUs that support AVX-512BW.

The increased blocksize and optimized hardware of CPUs with AVX-512BW offer significant performance improvements for parallelized applications such as icgrep.

With icgrep unchanged, `BlockSize=512` tested as 6.8% faster than `BlockSize=256`.

In addition to these base advantages, our changes to icgrep leverage new AVX-512BW intrinsics to further increase throughput and reduce run time.

With our changes, `BlockSize=512` tested as 18.4% faster than `BlockSize=256`.

# Conclusion

### Lessons Learned

### Future Work
