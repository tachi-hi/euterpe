#ifndef MY_FFT_H
#define MY_FFT_H

#include<complex>
#include<cstdio>
#include<vector>
#include<iostream>

namespace FFT{
  template<class S, class T>
  void fft__ (S *input, T *output, int length, bool flag);

  template<class S, class T>
  void forward (S *input, T *output, int length) {fft__(input, output, length, true); };
  template<class S, class T>
  void inverse (S *input, T *output, int length) {fft__(input, output, length, false); };

  inline void
    cast_copy(std::complex<double> *x, double *y, int length){
      for(int i = 0; i < length; i++){
        y[i] = x[i].real();
      }
    }
  inline void
    cast_copy(std::complex<double> *x, std::complex<double> *y, int length){
      for(int i = 0; i < length; i++){
        y[i] = x[i];
      }
    }
  inline void
    cast_copy(std::complex<float> *x, float *y, int length){
      for(int i = 0; i < length; i++){
        y[i] = x[i].real();
      }
    }
  inline void
    cast_copy(std::complex<float> *x, std::complex<float> *y, int length){
      for(int i = 0; i < length; i++){
        y[i] = x[i];
      }
    }
  inline void
    cast_copy(std::complex<double> *x, float *y, int length){
      for(int i = 0; i < length; i++){
        y[i] = x[i].real();
      }
    }
  inline void
    cast_copy(std::complex<double> *x, std::complex<float> *y, int length){
      for(int i = 0; i < length; i++){
        y[i] = x[i];
      }
    }
  inline void
    cast_copy(std::complex<float> *x, double *y, int length){
      for(int i = 0; i < length; i++){
        y[i] = x[i].real();
      }
    }
  inline void
    cast_copy(std::complex<float> *x, std::complex<double> *y, int length){
      for(int i = 0; i < length; i++){
        y[i] = x[i];
      }
    }
};

template<class S, class T>
void FFT::fft__(S *input, T *output, int length, bool flag){
  if(length < 2) return;
  for(int i = 1; i < length * 2; i *= 2){
    if(i == length ){
      break;
    }else if( i > length ){
      fprintf(stderr, "Error: at fft.cpp. Size of input data is illegal");
      return;
    }
  }
  double sign = flag ? 1.0 : -1.0;

  std::vector<std::complex<double> > tmp(length);
  for(int i = 0; i < length; i++){
    tmp[i] = input[i];
  }

  int xp2 = length;
  for(int iter = 1; iter < length*2; iter*=2){
  int xp = xp2;
    xp2 = xp / 2;
    double w = M_PI / xp2;

    for(int k = 0; k < xp2; k++){
      double arg = k * w;
      std::complex<double> ww( cos(arg), sign * sin(arg) );
      int i = k - xp;
      int j, p, q;
      for(j = xp, p = i + xp, q = i + xp + xp2; j <= length; j += xp, p += xp, q += xp){
        std::complex<double> t = tmp[p] - tmp[q];
	tmp[p] += tmp[q];
	tmp[q] = t * ww;
      }
    }
  }

  int j1 = length / 2;
  int j2 = length - 1;
  int i = 1, j = 1;
  int p;
  for(i = 1, p = 0; i <= j2; i++, p++){
    if( i < j ){
      std::complex<double> tmp_;
      tmp_ = tmp[p];
      tmp[p] = tmp[j - 1];
      tmp[j - 1] = tmp_;
    }
    int k = j1;
    while( k < j ){
      j -= k;
      k /= 2;
    }
    j += k;
  }

  cast_copy(&(tmp[0]), output, length);

  if(flag){
    ;
  }else{
    double w = static_cast<double>(length);
    for(i = 0, p = 0; i < length; i++, p++)
      output[p] /= w;
  }
  return;
}

#endif
