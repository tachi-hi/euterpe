#pragma once

// 信号処理の内部精度切り替え
//
// デフォルト: double
// float に切り替えるには CMake オプション -DEUTERPE_USE_FLOAT=ON を指定
// FFTW API（fftw_* / fftwf_*）も fftw_traits.h で自動切り替え

#ifdef EUTERPE_USE_FLOAT
using Scalar = float;
#else
using Scalar = double;
#endif
