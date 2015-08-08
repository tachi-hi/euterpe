/*
  wav file io

  2009年7月
*/

#include<iostream>
#include<fstream>
#include<cstdio>
#include<cstdlib>

#include<cmath>
#include<vector>

#include<string>
#include<algorithm>

namespace wav{
  //WAVファイルのヘッダ。
  struct Header{
    char riff[4];
    unsigned int filesize;
    char wavefmt[8];
    unsigned int waveformat;
    unsigned short int pcm;
    unsigned short int n_channel;
    unsigned int sampling_rate;
    unsigned int bytes_per_second;
    unsigned short int block_per_sample;
    unsigned short int bits_per_sample;
    char data[4];
    unsigned int n_byte;
  };

  //1チャンネル信号。
  class MonoChannel{
  public:
    MonoChannel(){}
    void init(int n){
      buffer_size = n;
      data = new double[n]; //バッファの大きさは決めてしまう。これ以上をストックしておくことはできない。
      current_point = 0;
    }
    ~MonoChannel(){
      delete []data;
    }
    void insert(double x){
      data[current_point] = x;
      current_point = (current_point < buffer_size - 1 ? current_point + 1 : 0);
    };
    double& operator[](int i){
      return data[i + current_point < buffer_size ? i + current_point : i + current_point - buffer_size];
    }
  private:
    double *data;
    int current_point; //リングバッファを管理するために使う。
    int buffer_size;
  };
  //多チャンネル信号 → 1チャンネル信号をたくさん保持する。
  class Signal{
  public:
    Signal(){}
    void init(int n_channel, int buf_size){
      buffer_size = buf_size;
      data = new MonoChannel[n_channel];
      for(int i = 0; i < n_channel; i++){
        data[i].init(buf_size);
      }
    }
    ~Signal(){
      delete []data;
    }
    int length(){return buffer_size;}//こんな関数必要だろうか？
    MonoChannel& operator[](int n){return data[n];}
  private:
    MonoChannel *data;
    int buffer_size;
  };
  //実数化，量子化の関数。
  //unsigned charやsigned shortから，実数に変換する関数。
  template<typename T> double realization(T){return 0.0;}; //この形では使わないが，C++の文法上必要なので書いてあるだけ。
  template<> double realization(unsigned char x){return static_cast<double>(x + 128) / 256.0;};
  template<> double realization(signed short x) {return static_cast<double>(x)/32768.0;};
  //doubleから，それぞれのunsigned charや，signed shortに戻す関数。
  template<typename T> T quantize(double){;}; //この形では使わないが，C++の文法上必要なので書いてある。
  template<> unsigned char quantize<unsigned char>(double x){return static_cast<unsigned char>(x * 256 - 128);};
  template<> signed short  quantize<signed short> (double x){return static_cast<signed short> (x * 32768);};
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//wavistreamクラス
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class wavistream{
public:
  wavistream(const char*, int);
  ~wavistream();
  void read(int n){if(header.bits_per_sample == 8)  __read<unsigned char>(n); else  __read<signed short>(n);};
  void copy(double* x, int ch, int n){for(int i = 0; i < n; i++)x[i] = signal[ch][i];};//とりあえずこうしておく。
  bool eof(){return feof(fp);}
  wav::Header header;
private:
  wav::Signal signal;
  std::string filename;
  FILE *fp;
  int current_seek; //現在ファイルの中のどの地点を読み込んでいるのかを記憶。
  template<typename T> void __read(int n);
};
wavistream::wavistream(const char *fn, int buffer_size){
  //ファイルポインタを取得
  filename = fn;
  fp = fopen(filename.c_str(), "r");
  if( !fp ){
    std::cerr << "cannot open " << filename << "!" <<std::endl;
    exit(1);
  }
  //ヘッダの読み込み。
  fread(&header, sizeof(header), 1, fp);
  current_seek = 0;//sizeof(header); 後者は間違い。ここが間違っているバージョンが存在するので要注意
  signal.init(header.n_channel, buffer_size);
}
wavistream::~wavistream(){
  fclose(fp);
}
template<typename T> void // unsigned char or signed short
wavistream::__read(int n){
  T tmp;
  double tmp_double;
  //頭出し
  fseek(fp, sizeof(wav::Header) + current_seek, SEEK_SET);
  //データの読み込み。wavファイルではチャンネル順に並んでいる。
  for(int i = 0; i < n ; i++){
    for(int ch = 0; ch < header.n_channel; ch++){
      if(!eof()){
        //オーバーランしないように。もし遅いようならネストの順番を見直す。
        fread(&tmp, sizeof(T), 1, fp);
        tmp_double = wav::realization(tmp);
        signal[ch].insert( tmp_double );
      }else{
        signal[ch].insert( 0.0 );
      }
    }
  }
  current_seek += n * header.n_channel * sizeof(T);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//wavistreamクラスおわり。
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//wavostreamクラス
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//まだ設計が良くない。wavを継承するモデルでいいのか？readとの対称性が必要ではないか？→継承はやめた。
class wavostream{
public:
  wavostream(const char*, int, int);
  ~wavostream();
  void set(double*, int, int);
  void write(int n){if(header.bits_per_sample == 8)  __write<unsigned char>(n); else  __write<signed short>(n);};;
  void write_header();
  wav::Header header;
  
private:
  wav::Signal signal;
  std::string filename;
  FILE *fp;
  template<typename T> void __write(int);
  int signal_length;
  int buffer_size;
};

wavostream::wavostream(const char *fn, int ch, int buffer_size){
  //ファイルポインタを取得
  filename = fn;
  fp = fopen(filename.c_str(), "w");
  if( !fp ){
    std::cerr << "cannot open " << filename << "!" <<std::endl;
    exit(1);
  }
  signal_length = 0;
  char tmp[sizeof(wav::Header)];
  //ヘッダ部分は先に0で埋めておく。
  fwrite(tmp, 1, sizeof(wav::Header), fp);
  signal.init(ch, buffer_size);
  header.n_channel = ch;
  this->buffer_size = buffer_size;
}
wavostream::~wavostream(){
  write_header();
  fclose(fp);
}

void wavostream::set(double *x, int ch, int n){
  //まずはサチュレーションをなくす。最大値を取得→これをやると，不自然。
//  double max = *std::max_element(&(x[0]), &(x[n]));
  for(int t = 0; t < n; t++){
    if(x[t] > 1.0){
      signal[ch].insert(1.0);
    }else if(x[t] < -1.0){
      signal[ch].insert(-1.0);
    }else{
      signal[ch].insert(x[t]);
    }
  }
}

//データを実際に書き込み。
//書くときはデータを頭からではなくおしりから書く。
template<typename T> void // unsigned char or signed short
wavostream::__write(int n){
  for(int j = buffer_size - n; j < buffer_size; j++){
    for(int ch = 0; ch < header.n_channel; ch++){
      //doubleの系列を，unsigned char, signed shortに量子化する。
      T tmp = wav::quantize<T>(signal[ch][j]);
      fwrite(&tmp, sizeof(T), 1, fp);
    }
  }
//  current_seek += n * header.n_channel * sizeof(T);
  //ヘッダに変更を加える。
  signal_length += n;
}

void wavostream::write_header(void){
  if(header.sampling_rate == 0 || header.n_channel == 0 || header.bits_per_sample == 0){
    std::cerr << "sampling_rate, n_channel & bit_rate are not set!" << std::endl;
    exit(1);
  }

  //ヘッダの各要素に片っぱしから代入。
  header.riff[0] = 'R';
  header.riff[1] = 'I';
  header.riff[2] = 'F';
  header.riff[3] = 'F';
  header.filesize = (header.bits_per_sample / 8) * signal_length * header.n_channel + 36;
  header.wavefmt[0] = 'W';
  header.wavefmt[1] = 'A';
  header.wavefmt[2] = 'V';
  header.wavefmt[3] = 'E';
  header.wavefmt[4] = 'f';
  header.wavefmt[5] = 'm';
  header.wavefmt[6] = 't';
  header.wavefmt[7] = ' ';
  header.waveformat = header.bits_per_sample;
  header.pcm = 1;
  header.n_channel = header.n_channel;
  header.sampling_rate = header.sampling_rate;
  header.bytes_per_second = (header.bits_per_sample / 8) * header.n_channel * header.sampling_rate;
  header.block_per_sample = (header.bits_per_sample / 8) * header.n_channel;
  header.bits_per_sample = header.bits_per_sample;
  header.data[0] = 'd';
  header.data[1] = 'a';
  header.data[2] = 't';
  header.data[3] = 'a';
  header.n_byte = (header.bits_per_sample / 8) * signal_length * header.n_channel; //この辺の計算は見直す必要がある。
  fseek(fp, 0, SEEK_SET);
  fwrite(&header, sizeof(header), 1, fp);

}

