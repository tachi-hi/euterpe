#pragma once

#include <complex>
#include <vector>
#include <algorithm>  // std::transform, std::swap, std::for_each, std::copy_n
#include <iostream>
#include <cmath>
#include "scalar.h"

namespace FFT {

  template<class S, class T>
  void fft__(S* input, T* output, int length, bool flag);

  template<class S, class T>
  void forward(S* input, T* output, int length) { fft__(input, output, length, true);  }
  template<class S, class T>
  void inverse(S* input, T* output, int length) { fft__(input, output, length, false); }

  // complex<T> → complex<U>: 型変換コピー
  template<typename T, typename U>
  inline void cast_copy(const std::complex<T>* x, std::complex<U>* y, int n) {
    std::transform(x, x + n, y, [](const std::complex<T>& v) { return std::complex<U>(v); });
  }

  // complex<T> → 実数 U: 実部のみ取り出す
  template<typename T, typename U>
  inline void cast_copy(const std::complex<T>* x, U* y, int n) {
    std::transform(x, x + n, y, [](const std::complex<T>& v) { return static_cast<U>(v.real()); });
  }

} // namespace FFT


template<class S, class T>
void FFT::fft__(S* input, T* output, int length, bool flag) {
  if (length < 2) return;

  // 2 の冪乗チェック
  if ((length & (length - 1)) != 0) {
    std::cerr << "Error: at fft.hpp. Size of input data is not a power of 2." << std::endl;
    return;
  }

  const Scalar sign = flag ? Scalar(1) : Scalar(-1);

  // 入力を complex<Scalar> の作業バッファにコピー
  std::vector<std::complex<Scalar>> tmp(length);
  std::transform(input, input + length, tmp.begin(),
    [](const S& v) -> std::complex<Scalar> { return std::complex<Scalar>(v); });

  // バタフライ演算
  int xp2 = length;
  for (int iter = 1; iter < length * 2; iter *= 2) {
    int xp = xp2;
    xp2 = xp / 2;
    auto w = Scalar(M_PI) / xp2;
    for (int k = 0; k < xp2; k++) {
      auto arg = k * w;
      std::complex<Scalar> ww(std::cos(arg), sign * std::sin(arg));
      int i = k - xp;
      for (int j = xp, p = i + xp, q = i + xp + xp2;
           j <= length;
           j += xp, p += xp, q += xp) {
        auto t = tmp[p] - tmp[q];
        tmp[p] += tmp[q];
        tmp[q] = t * ww;
      }
    }
  }

  // ビット逆順並べ替え
  int j1 = length / 2;
  int j2 = length - 1;
  for (int i = 1, j = 1, p = 0; i <= j2; i++, p++) {
    if (i < j) std::swap(tmp[p], tmp[j - 1]);
    int k = j1;
    while (k < j) { j -= k; k /= 2; }
    j += k;
  }

  cast_copy(tmp.data(), output, length);

  // 逆変換の場合は長さで正規化
  if (!flag) {
    const auto scale = Scalar(1) / length;
    std::for_each(output, output + length, [scale](auto& v) { v *= scale; });
  }
}
