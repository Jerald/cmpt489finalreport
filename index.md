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

### Intrinsic Discovery

One of the first real issues that arose was a pretty simple one: how do we *use* an intrinsic? We had found a useful intrinsic through the Intel Intrinsics Guide, but we had no clue how to use it in LLVM. The x86 opcode didn't seem to be of much use, and the Intel Intrinsic name was just as meaningless to LLVM.


### LLVM


# Implementation Details

### hsimd_packh and hsimd_packl

### esimd_popcount

### esimd_bitspread


# Implementation Evaluation


# Conclusion

### Lessons Learned

### Future Work