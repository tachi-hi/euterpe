// signalProcessingLibrary.cpp
//
// PARCOR クラスのメソッド実装。
// refsig / corref / convol / sigcor / SigRef2 の自由関数は
// signalProcessingLibrary.h にテンプレートとして定義されている。
//
// 1981 S.Sagayama (original Fortran/C)
// 整理 橘

#include "signalProcessingLibrary.h"
#include <algorithm>

void PARCOR::init(int p_){
	p = p_;
	state.resize(p);
}

// PARCOR all-pole filter（内部状態 state を保持するメンバ版）
double PARCOR::refsig(const double* ref, const double xin){
	double y = xin + ref[p] * state[p - 1];
	for(int i = p - 1; i > 0; i--) {
		y          += ref[i] * state[i - 1];
		state[i]    = state[i - 1] - ref[i] * y;
	}
	state[0] = y;
	return y;
}

void PARCOR::corref(double* cor, double* alf, double* ref){
	ref[1]       = cor[1];
	alf[1]       = -ref[1];
	double resid = (1.0 - ref[1]) * (1.0 + ref[1]);
	for(int i = 2; i <= p; i++){
		double r = cor[i];
		for(int j = 1; j < i; j++) r += alf[j] * cor[i - j];
		alf[i] = -(ref[i] = (r /= resid));
		int j = 0, k = i;
		while(++j <= --k){
			double a = alf[j];
			alf[j]  -= r * alf[k];
			if(j < k) alf[k] -= r * a;
		}
		resid *= (1.0 - r) * (1.0 + r);
	}
}

void PARCOR::convol(const double* u, const int m, const double* v, double* w, const int n){
	for(int i = 0; i < n; i++){
		double sum = 0.0;
		for(int j = 0; j < std::min(i + 1, m); j++) sum += u[j] * v[i - j];
		w[i] = sum;
	}
}
