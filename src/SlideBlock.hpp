
#ifndef SLIDE_BLOCK_HEADER
#define SLIDE_BLOCK_HEADER

#include<cstdio>
#include<vector>

template<typename T>
class SlideBlock{
public:
  SlideBlock(int t, int k);
  ~SlideBlock();

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
  T* data;

private:
  int current_point;
  T** data_alias;
};

template<typename T>
SlideBlock<T>::SlideBlock(int n, int k)
     :n_time(n),
      n_freq(k),
      current_point(0)
{
  data = new T[n * k];
  data_alias = new T*[n];
  for(int i = 0; i != n; i++){
    data_alias[i] = &(data[i * k]);
  }
  for(int i = 0; i < n * k; i++){
    data[i] = 0;
  }
}

template<typename T>
SlideBlock<T>::~SlideBlock(){
  delete[] data;
  delete[] data_alias;
}

template<typename T>
T SlideBlock<T>::at(int t, int k){
  t = (t - current_point);
  if(t < 0){
    t = t + n_time;
  }
  if(0 <= t < n_time && 0 <= k < n_freq){
    return data_alias[t][k];
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
  return data_alias[t];
}

template<typename T>
void SlideBlock<T>::apply(T(*func)(T)){
  for(int i = 0; i < n_time * n_freq; i++)
    data[i] = func(data[i]);
}

template<typename T>
void SlideBlock<T>::push(T* in){
  for(int i = 0; i != n_freq; i++)
    data_alias[current_point][i] = in[i];
  current_point++;
  if(current_point == n_time)
    current_point = 0;
}
template<typename T>
void SlideBlock<T>::push(std::vector<T> in){
  if(in.size() != n_freq){
    printf("Error at SlideBlock.hpp, push(std::vector<T>): size of std::vector is invalid.\n");
    exit(1);
  }
  for(int i = 0; i != n_freq; i++)
    data_alias[current_point][i] = in[i];
  current_point++;
  if(current_point == n_time)
    current_point = 0;
}

template<typename T>
void SlideBlock<T>::pop(T* out){
  for(int i = 0; i != n_freq; i++)
    out[i] = data_alias[current_point][i];
}

#endif
