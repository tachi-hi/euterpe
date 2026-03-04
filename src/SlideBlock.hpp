
#pragma once

#include <vector>
#include <algorithm>
#include <cstdio>

template<typename T>
class SlideBlock{
public:
  SlideBlock(int t, int k);
  ~SlideBlock() = default;

  void apply(T(*)(T));
  T at(int t, int k);
  T* operator[](int t);
  void push(T *in);
  void push(std::vector<T> in);
  void pop(T *out);
  std::vector<T> pop();

protected:
  int n_time;
  int n_freq;

private:
  int current_point;
  std::vector<T>   data_;
  std::vector<T*>  data_alias_;
};

template<typename T>
SlideBlock<T>::SlideBlock(int n, int k)
     :n_time(n),
      n_freq(k),
      current_point(0),
      data_(n * k, T{}),
      data_alias_(n)
{
  for(int i = 0; i != n; i++){
    data_alias_[i] = &data_[i * k];
  }
}

template<typename T>
T SlideBlock<T>::at(int t, int k){
  t = (t - current_point);
  if(t < 0){
    t = t + n_time;
  }
  if(0 <= t && t < n_time && 0 <= k && k < n_freq){
    return data_alias_[t][k];
  }else{
    printf("Error at SlideBlock.hpp\nToo large index: time index: %d and frequency index %d", t, k);
    exit(1);
  }
}

template<typename T>
T* SlideBlock<T>::operator[](int t){
  t += current_point;
  if(t >= n_time){
    t -= n_time;
  }
  return data_alias_[t];
}

template<typename T>
void SlideBlock<T>::apply(T(*func)(T)){
  for(auto& v : data_)
    v = func(v);
}

template<typename T>
void SlideBlock<T>::push(T* in){
  std::copy_n(in, n_freq, data_alias_[current_point]);
  if(++current_point == n_time)
    current_point = 0;
}

template<typename T>
void SlideBlock<T>::push(std::vector<T> in){
  if((int)in.size() != n_freq){
    printf("Error at SlideBlock.hpp, push(std::vector<T>): size of std::vector is invalid.\n");
    exit(1);
  }
  std::copy(in.begin(), in.end(), data_alias_[current_point]);
  if(++current_point == n_time)
    current_point = 0;
}

template<typename T>
void SlideBlock<T>::pop(T* out){
  std::copy_n(data_alias_[current_point], n_freq, out);
}
