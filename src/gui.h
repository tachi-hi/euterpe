/*******************************************************************/
//	GUI
//
// (c) 2010 Aug. Hideyuki Tachibana. tachibana@hil.t.u-tokyo.ac.jp
/*******************************************************************/

#pragma once
#include "myMutex.h"
#include <cstdio>
#include <pthread.h>

class GUI{
 public:

  GUI(int, char**);
  ~GUI();

  // GUI parameters
  // myMutex chould be initialized in constructer 
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
  FILE * fpipe;
	int stop;
	int quit;
  void getGUIparameters_(void);
  static void* getGUIparameters(void* arg){reinterpret_cast<GUI *>(arg)->getGUIparameters_(); return 0;};

  pthread_t GUI_thread;
  pthread_attr_t attr;

};
