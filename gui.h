/*******************************************************************/
//	GUI
//
// (c) 2010 Aug. Hideyuki Tachibana. tachibana@hil.t.u-tokyo.ac.jp
/*******************************************************************/

#pragma once
#include "myMutex.h"
#include <cstdio>
#include <pthread.h>

#include <gtk/gtk.h>

class Button{
 public:
	Button(char* label){button = gtk_button_new_with_label(label);}
	~Button(){}
 private:
	GtkWidget *button;
//	(void*func)(void);
};

//template<typename T>
class Slider{
 public:
	Slider(){};
	~Slider(){};
	void link_value(int* link);
 private:
	GtkWidget slider;
	int min;
	int max;
	int def_val;
	void func();
};

template<typename T>
class Radio{
 public:
	Radio(){}
	~Radio(){}
	 void link_value(T* link);
 private:
	GtkWidget slider;
	int min;
	int max;
	int def_val;
	void func();
};

class GUI{
 public:
 
  GUI(int, char**);
  ~GUI();
  
  // GUI parameters
  // myMutex chould be initialized in constructer myMutexの仕様より。
  myMutex<int> key;
  myMutex<int> tempo;
  myMutex<int> volume;
  enum RunState{is_ready, is_start, is_stop};
  myMutex<RunState> runState;
  
  // GUI panel
	void start (void);
  bool getstop(void){return stop == -1;}
  bool getquit(void){return quit == -1;}
 private:
  FILE * fpipe; //パイプでデータを受け取る役割
	int stop;
	int quit;
  void getGUIparameters_(void);
  static void* getGUIparameters(void* arg){reinterpret_cast<GUI *>(arg)->getGUIparameters_(); return 0;};

  pthread_t GUI_thread;
  pthread_attr_t attr;

	// gtk
	GtkWidget *window;
	void createWindow(void);

	GtkWidget *startButton;
	GtkWidget *quitButton;

/*
	static void * callback(void *arg){reinterpret_cast<GUI*>(arg)->callback_();return 0;}

	void callback_(void){
		gtk_main();
	}
*/
};

