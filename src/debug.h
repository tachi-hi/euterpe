#include <iostream>

#ifndef DEBUG_H
#define DEBUG_H

// comment out unless debug mode

template<typename T>
inline void DBGMSG(T m){
	using namespace std;
	cerr << m << endl;
}

#endif
