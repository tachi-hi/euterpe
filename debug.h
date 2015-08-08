#include <iostream>

#ifndef DEBUG_H
#define DEBUG_H

//デバッグしないときはコメントアウト

template<typename T>
inline void DBGMSG(T m){
	using namespace std;
	cerr << m << endl;
}

#endif

