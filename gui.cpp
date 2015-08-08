/*******************************************************************/
// GUI
//
// (c) 2010 Aug. Hideyuki Tachibana. tachibana@hil.t.u-tokyo.ac.jp
/*******************************************************************/

#include "gui.h"
#include <cstdlib>
#include <cstring>
#include <iostream>

GUI::GUI(int argc, char **argv)
	:	key(0),
		tempo(100),
		volume(100),
		quit(0),
		stop(0),
		runState(is_ready)
{
	const char *command="wish TkTempoConv3.tcl";

	if ( !(fpipe = (FILE*)popen(command,"r")) ){
		perror("Problems with pipe");
		exit(1);
	}
	std::cerr << "created GUI." << std::endl;

//	pthread_create(&GUI_thread, &attr, (void*(*)(void*))callback, this);
}

GUI::~GUI(){
	pclose(fpipe);  
}


void GUI::start(void){
		pthread_attr_init(&attr);
		pthread_attr_setschedpolicy(&attr, SCHED_RR);
		pthread_create(&GUI_thread, &attr, (void*(*)(void*))GUI::getGUIparameters, this);
//		gtk_main();
}

void GUI::getGUIparameters_(void){
	while(1){
		usleep(1000);
		static char line[1024];
		static char label[32];
		int value;
		if(fgets(line, 1023, fpipe) != NULL){
			sscanf(line, "%s %d\n", label, &value);
			if(strcmp(label, "key") == 0){
				key.set(value);
			}else if (strcmp(label, "tempo") == 0){
				tempo.set(value);
			}else if (strcmp(label, "volume") == 0){
				volume.set(value);
			}else if (strcmp(label, "stop") == 0){
				 stop = -1;
			}else if (strcmp(label, "quit") == 0){
				 quit = -1;
			}
		std::cerr << label << " " << value << std::endl;
		}
	}
}

