#ifndef IDISA_AVX_BUILDER_H
#define IDISA_AVX_BUILDER_H

/*
 *  Copyright (c) 2015 International Characters.
 *  This software is licensed to the public under the Open Software License 3.0.
*/

#include <IR_Gen/idisa_sse_builder.h>
#include <llvm/Support/raw_ostream.h>

namespace IDISA {

class IDISA_AVX_Builder : public IDISA_SSE2_Builder {
public:

    IDISA_AVX_Builder(llvm::LLVMContext & C, unsigned vectorWidth, unsigned stride)
    : IDISA_Builder(C, vectorWidth, stride)
    , IDISA_SSE2_Builder(C, vectorWidth, stride)
    {

    }

    virtual std::string getBuilderUniqueName() override;

    llvm::Value * hsimd_signmask(unsigned fw, llvm::Value * a) override;

    ~IDISA_AVX_Builder() {}

};

class IDISA_AVX2_Builder : public IDISA_AVX_Builder {
public:

    IDISA_AVX2_Builder(llvm::LLVMContext & C, unsigned vectorWidth, unsigned stride)
    : IDISA_Builder(C, vectorWidth, stride)
    , IDISA_AVX_Builder(C, vectorWidth, stride) {

    }

    virtual std::string getBuilderUniqueName() override;
    llvm::Value * hsimd_packh(unsigned fw, llvm::Value * a, llvm::Value * b) override;
    llvm::Value * hsimd_packl(unsigned fw, llvm::Value * a, llvm::Value * b) override;
    llvm::Value * esimd_mergeh(unsigned fw, llvm::Value * a, llvm::Value * b) override;
    llvm::Value * esimd_mergel(unsigned fw, llvm::Value * a, llvm::Value * b) override;
    llvm::Value * hsimd_packh_in_lanes(unsigned lanes, unsigned fw, llvm::Value * a, llvm::Value * b) override;
    llvm::Value * hsimd_packl_in_lanes(unsigned lanes, unsigned fw, llvm::Value * a, llvm::Value * b) override;
    std::pair<llvm::Value *, llvm::Value *> bitblock_add_with_carry(llvm::Value * a, llvm::Value * b, llvm::Value * carryin) override;
    std::pair<llvm::Value *, llvm::Value *> bitblock_indexed_advance(llvm::Value * a, llvm::Value * index_strm, llvm::Value * shiftin, unsigned shift) override;
    llvm::Value * hsimd_signmask(unsigned fw, llvm::Value * a) override;

    ~IDISA_AVX2_Builder() {}
};

class IDISA_AVX512F_Builder : public IDISA_AVX2_Builder {
public:

    IDISA_AVX512F_Builder(llvm::LLVMContext & C, unsigned vectorWidth, unsigned stride)
    : IDISA_Builder(C, vectorWidth, stride)
    , IDISA_AVX2_Builder(C, vectorWidth, stride) {

        getHostCPUFeatures();
    }

    //Implemented
    virtual std::string getBuilderUniqueName() override;
    llvm::Value * hsimd_packh(unsigned fw, llvm::Value * a, llvm::Value * b) override;
    llvm::Value * hsimd_packl(unsigned fw, llvm::Value * a, llvm::Value * b) override;
    llvm::Value * simd_popcount(unsigned fw, llvm::Value * a) override;
    llvm::Value * esimd_bitspread(unsigned fw, llvm::Value * bitmask) override;
    llvm::Value * hsimd_signmask(unsigned fw, llvm::Value * a) override;

    ~IDISA_AVX512F_Builder() {}

private:

    struct Features {
        //not an exhaustive list, can be extended if needed
        bool hasAVX512CD = false;
        bool hasAVX512BW = false;
        bool hasAVX512DQ = false;
        bool hasAVX512VL = false;
        bool hasAVX512VBMI = false;
        bool hasAVX512VBMI2 = false;
        bool hasAVX512VPOPCNTDQ = false;
    };

    void getHostCPUFeatures() {
        llvm::StringMap<bool> features;
        if (llvm::sys::getHostCPUFeatures(features)) {
            hostCPUFeatures.hasAVX512CD = features.lookup("avx512cd");
            hostCPUFeatures.hasAVX512BW = features.lookup("avx512bw");
            hostCPUFeatures.hasAVX512DQ = features.lookup("avx512dq");
            hostCPUFeatures.hasAVX512VL = features.lookup("avx512vl");


            //hostCPUFeatures.hasAVX512VBMI, hostCPUFeatures.hasAVX512VBMI2,
            //hostCPUFeatures.hasAVX512VPOPCNTDQ have not been tested as we
            //did not have hardware support. It should work in theory (tm)

            hostCPUFeatures.hasAVX512VBMI = features.lookup("avx512vbmi");
            hostCPUFeatures.hasAVX512VBMI2 = features.lookup("avx512vbmi2");
            hostCPUFeatures.hasAVX512VPOPCNTDQ = features.lookup("avx512vpopcntdq");
        }
    }

    Features hostCPUFeatures;
};


}
#endif // IDISA_AVX_BUILDER_H
