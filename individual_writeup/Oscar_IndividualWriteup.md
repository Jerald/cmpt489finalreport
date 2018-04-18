# Oscar's Individual Write-up

This project has been one of the most interesting, difficult, and educational experiences I've had in a long time. Right from the start we were out of our depth, but we quickly made the best of the situation and found our footing, however unstable.

### My Focuses

Throughout the project, my role has mostly been doing research into how to use LLVM's tools in the ways we needed. This was in doing things such as finding how to translate Intel Intrinsics to LLVM Intrinsics, identifying the needed parameters for LLVM Intrinsics, and finding methods of identifying slow code. I also was primarily in charge of our operational organization, such as our usage of gitlab for source control or finding ways to use the server to our advantage.

### LLVM Insights

A large amount of my time was spent working directly with LLVM, as one could probably figure out seeing as the parts of the updates concerning it were always done by me. A lot of that saw me developing local tools to aid our work, such as the `findintrinsic` script, or simply being our team's expert on these topics.

The initial work to build `findintrinsic` involved a lot of time and research into highly esoteric aspects of LLVM. For most of the time I was working on it, I was unable to find anything at all online. The resulting information from the endeavour ended up coming almost entirely from my own work. This issue of lacking information only became more profound when I started looking into other problems like the selection failure we had.

In diagnosing the selection issue, this was the "Nine Circles of HeLLVM" section from our update two, I spent a great deal of time scratching my head without any idea of how to proceed. Almost all of the discoveries were very iterative and minor, slowly building me up to the final conclusion. As previously mentioned, there was an extreme lack of information available online in regards to what I was finding. As far as I can tell, it's quite possible that I am one of extremely few people to have gone through the journey of discovery I did.

By the time I'd been through the other two adventures in LLVM, I'd become far more adept at navigating its confusing waters. When we had the issues of using incorrect types in an LLVM intrinsic, I ended up being able to diagnose and track down the problem far faster than I had with the other two. This seemed to be due to a mixture of already knowing places to look and having a better grasp of what exactly I was looking for in the end. In fact, this investigation ended up not using any online resources, aside from the LLVM tablegen language reference. Compared to the previous bits of research, I consider that somewhat of a success.

### Organization

Although not really all that related to our outcomes, I spent a fair amount of time on organizing how we'd end up working together. This included setting up a gitlab repo for our working copy, and dealing with importing icgrep updates from svn into our codebase.

As well, I ended up spending far too much time on looking into ways for us to edit our code locally, but have it instantly available to test on the server. I had originally designed these methods for use by my groupmates, but in the end I was the only one who used them. My final solution ended up being far simpler than what I'd tried in the past, as I simply used the common unix tool `rsync` to send files over to the server every time I saved. The result of this was that I could also immediately trigger a build and have a modified icgrep testable in a very short period of time.

### Code Analysis

As our goal was for maximum performance, analyzing differences in compiled code was pretty important for us. This was used in identifying how we'd improved an operation, finding ways to improve other operations, or for finding cases where improvement was unnecessary.

Although I didn't make any great strides per se, I ended up becoming quite proficient in a number of methods for this sort of investigation. This included things like compiling icgrep in debug mode to spot more problems or knowing how to dump the IR at a given stage to see what things look like. Because I had this knowledge, I ended up spending a good deal of time doing analysis like this on various operations in an attempt to find inefficiencies. While not all of them panned out, it did provide interesting insight into how what we wrote became actual IR or assembly.

### Conclusion

Unlike my groupmates, I didn't end up specifically authoring any operations which ended up in the final version. Though I did try to work on a few, they ended up either having ridiculous issues (such as the LLVM selection problem) or simply being the most optimized they could reasonably be. But just because I didn't personally contribute to the final source code, that doesn't mean I wasn't an integral part of our team. Although this is just speaking for myself, I'd like to think that without the work I put into LLVM, our project wouldn't have been nearly as big a success.