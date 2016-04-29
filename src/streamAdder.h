/***************************************************
 * streamAdder
 *
 * (c) 2010 Aug. Hideyuki Tachibana
 * tachibana@hil.t.u-tokyo.ac.jp
 **************************************************/

#pragma once

#include "streamBuffer.h"
#include <vector>
#include"debug.h"
#include <unistd.h> //usleep
#include "gui.h"


class StreamAdder{
 public:
	StreamAdder(int n, GUI *panel_){
		cycle = n; // amount of data to process once
		panel = panel_;
		buffer_read   = new float[cycle];
		buffer_added = new float[cycle];
		pthread_attr_init(&attr);
		pthread_attr_setschedpolicy(&attr, SCHED_RR);
	};

	~StreamAdder(){
		delete[] buffer_read;
		delete[] buffer_added;
	};

	void add_input_stream(StreamBuffer<float>* tmp){
		streams.push_back(tmp);
		multiply.push_back(1.0);
	}

	void set_multiply(int n, double value){
		multiply.at(n) = value;
	}

	void set_output_stream(StreamBuffer<float>* tmp){
		output = tmp;
	}

	void start(void){
		pthread_create(&thread, &attr, (void*(*)(void*))callback, this);
	}

	static void* callback(void* arg){reinterpret_cast<StreamAdder*>(arg)->callback_();return 0;}

	void callback_(void){
		while(1){
			double tmp = (double)(panel->volume.get())/200.0;
			multiply[0] = tmp < 0.5 ? 1 + 0.5 - 1.0 * tmp : 4 * (1.0 - tmp) * (1.0 - tmp);
			multiply[1] = tmp < 0.5 ? 1 + 3.0 - 2.0 * tmp : 4 * (1.0 - tmp) * (1.0 - tmp); // harmonic louder
			multiply[2] = tmp < 0.5 ? 4 * tmp * tmp : 1 + 4 * (tmp - 0.5);

			for(int j = 0; j < cycle; j++){
				buffer_added[j] = 0.0;
			}

			for(int i = 0; i < (int)streams.size(); i++){
				while(!streams[i]->read_data(buffer_read, cycle)){
					usleep(100);
				} // if it fails, this while loops forever
	
				for(int j = 0; j < cycle; j++){
					buffer_added[j] += multiply[i] * buffer_read[j];
				}
			}
			output->push_data(buffer_added, cycle);
		}
	}

 private:
	std::vector<StreamBuffer<float>*> streams;
	std::vector<float> multiply;
	StreamBuffer<float> *output;
	int cycle;
	float *buffer_read;
	float *buffer_added;
	GUI *panel;

	pthread_t thread;
	pthread_attr_t attr;

};
