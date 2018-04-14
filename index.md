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

Of course, in a project like this there were bound to be some challenges. But we didn't predict quite this many challenges arising. Right at the beginning there was the obvious challenge of becoming acquainted with the project, but that was just the beginning of our journey.

### Intrinsic Locating and Usage

One of the first real issues that arose was a pretty simple one: how do we *use* an intrinsic? We had found a useful intrinsic through the Intel Intrinsics Guide, but we had no clue how to use it in LLVM. The x86 opcode didn't seem to be of much use, and the Intel Intrinsic name was just as meaningless to LLVM. That's when the digging began.

#### The First Breakthrough

So we had previously known that you can use the Intel Intrinsics in your code through adding the header `immintrin.h`. What we didn't know was how the intrinsics were defined in said header. Turns out, they're not defined as assembly black magic as we had previously thought, they're implemented in terms of these weird things called GCC Builtin's. A GCC Builtin is the closest analogue to an LLVM Intrinsic. Basically, it's some arbitrary operation that's defined internally by GCC.

Now this may not seem like it was that useful, and initially, it wasn't. But one time, while wallowing in our failure, we got annoyed and just tried searching through the entire icgrep source directory for an intrinsic. We first did it with the opcode, which turned up a mess of scariness we never touched again. But then we tried it again with that GCC Builtin we found. And that's where things started to turn around.

#### The Second Finding

By searching through the icgrep directory like that we turned up a number of things, most of them useless. But there were two lines which were of interest to us:

```cpp
return Intrinsic::x86_avx512_mask_pmov_wb_512;    // "__builtin_ia32_pmovwb512_mask"
GCCBuiltin<"__builtin_ia32_pmovwb512_mask">,
```

The comment on that first line was in fact the Builtin we were searching for. Given the context, we could only assume the thing called "Intrinsic" was probably the LLVM Intrinsic we were looking for. As it turns out, it was! With this we now had a semi-reliable, albeit tedious, method to find the LLVM Intrinsic that corresponds to an Intel Intrinsic.

#### The Third Discovery

But now that we have the name to call, how do we actually, well, *use* it in code? We don't really know the paramaters to pass it, only some vague hint from the Intel Intrinsics Guide. Well our first idea was to look back at the intrinsic implementation we found in `immintrin.h`. It turns out that along with the Builtin, there is some code actually calling it. That provided us a vague hint at the very least. We decided that since the LLVM Intrinsics are based off the GCC Builtin's, it made sense if they were called the same. And what did you know, it actually worked! We had code that ran and performed the operations we wanted. Except, as we soon realized, everything was not as it seemed.

#### The Fourth Revelation

Turns out, LLVM often wants different, and more specific, types for the parameters than GCC does. This was a hard problem to solve, because as of then we hadn't found any part of this actually in LLVM. Well, our saviour came in the form of that second line from above:

```cpp
GCCBuiltin<"__builtin_ia32_pmovwb512_mask">,
```

The file this came from is called a tablegen file. It's part of how LLVM generates targets, also known as backend. When we looked at the file the line came from, we found this:

```cpp
def int_x86_avx512_mask_pmov_wb_512 :
          GCCBuiltin<"__builtin_ia32_pmovwb512_mask">,
          Intrinsic<[llvm_v32i8_ty],
                    [llvm_v32i16_ty, llvm_v32i8_ty, llvm_i32_ty],
                    [IntrNoMem]>;
```

This of course was basically complete gibberish to us. All we could gather was that it defined something which looked basically like our intrinsic, and it was somehow connecting that to the GCC Builtin. Well after a long and painful investigation we don't have the time to get into, we eventually found out how to read these tablegen entries.

As it turns out, within `Intrinsic<>`, the first part in brackets is what the intrinsic returns and the second part in brackets is the *ordered list* of paramaters. That last part was the most important. With that we officially knew the types, and even their correct order, as needed by the intrinsic.

#### The Conclusion

Using everything we found out, we, in true compsci fashion, created a script to do all this hard work for us. Simply provided with an Intel Intrinsic, it will tell us the GCC Builtin, LLVM Intrinsic, source header, `immintrin.h` definition, and even the LLVM usage. Although being completely unrelated to any actual improvements in icgrep or Parabix, this was actually one of the most useful breakthroughs in our opinion. Because of this, we've included a cleaned up and optimized version of our script in appendix <INSERT APENDIX NUMBER HERE!>. We hope it can perhaps offer as much help, and save as much pain, for others as it has for us.

### LLVM

With all the talk about how much trouble we had with LLVM related things, you might think we've already covered our troubles with LLVM. I wish that were true, but this is only the beginning. We don't call it HeLLVM for nothing.




# Implementation Details

### hsimd_packh and hsimd_packl

### esimd_popcount

### esimd_bitspread


# Implementation Evaluation


# Conclusion

### Lessons Learned

### Future Work