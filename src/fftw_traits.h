#pragma once

// FFTW の double/float API 切り替えトレイツ
//
// Scalar = double のとき fftw_*  API を使用
// Scalar = float  のとき fftwf_* API を使用
//
// CMake オプション -DEUTERPE_USE_FLOAT=ON でfloatに切り替え。
// その際 FFTW3f (fftwf_* 関数) のリンクも CMakeLists.txt で切り替わる。

#include <fftw3.h>
#include "scalar.h"

template<typename T> struct FftwTraits;

template<> struct FftwTraits<double> {
    using plan    = fftw_plan;
    using complex = fftw_complex;

    static plan plan_dft_r2c_1d(int n, double* in, complex* out, unsigned flags)
        { return fftw_plan_dft_r2c_1d(n, in, out, flags); }
    static plan plan_dft_c2r_1d(int n, complex* in, double* out, unsigned flags)
        { return fftw_plan_dft_c2r_1d(n, in, out, flags); }
    static void execute(plan p)      { fftw_execute(p); }
    static void destroy(plan p)      { fftw_destroy_plan(p); }
    static void* alloc(size_t n)     { return fftw_malloc(n); }
    static void  free_(void* p)      { fftw_free(p); }
};

template<> struct FftwTraits<float> {
    using plan    = fftwf_plan;
    using complex = fftwf_complex;

    static plan plan_dft_r2c_1d(int n, float* in, complex* out, unsigned flags)
        { return fftwf_plan_dft_r2c_1d(n, in, out, flags); }
    static plan plan_dft_c2r_1d(int n, complex* in, float* out, unsigned flags)
        { return fftwf_plan_dft_c2r_1d(n, in, out, flags); }
    static void execute(plan p)      { fftwf_execute(p); }
    static void destroy(plan p)      { fftwf_destroy_plan(p); }
    static void* alloc(size_t n)     { return fftwf_malloc(n); }
    static void  free_(void* p)      { fftwf_free(p); }
};

// 現在の Scalar 型に対応する FFTW API
using Fftw = FftwTraits<Scalar>;
