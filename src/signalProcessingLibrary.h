
#pragma once

#include <vector>
#include <algorithm>
#include <cmath>

// -----------------------------------------------------------------------------
// PARCOR クラス（double 固定、内部状態を保持するフィルタ）
// -----------------------------------------------------------------------------
class PARCOR{
 public:
	PARCOR(){}
	~PARCOR(){}
	void init(int);
	double refsig(const double*, const double);
	void corref(double* cor,double* alf,double* ref);
	void convol(const double* u, const int m, const double* v, double* w, const int n );
 private:
	int p;
	std::vector<double> state;
};

// -----------------------------------------------------------------------------
// 自由関数 — テンプレート実装（Scalar = double / float に対応）
// -----------------------------------------------------------------------------

// PARCOR all-pole filter (refsig)
template<typename T>
inline T refsig(int p, const T* ref, T* state, T xin) {
	if (p <= 0) return xin;
	T y = xin + ref[p] * state[p - 1];
	for (int i = p - 1; i > 0; i--) {
		y         += ref[i] * state[i - 1];
		state[i]   = state[i - 1] - ref[i] * y;
	}
	state[0] = y;
	return y;
}

// 自己相関係数 → PARCOR係数変換
template<typename T>
inline void corref(int p, T* cor, T* alf, T* ref) {
	T resid, r, a;
	ref[1]  = cor[1];
	alf[1]  = -ref[1];
	resid   = (T(1) - ref[1]) * (T(1) + ref[1]);
	for (int i = 2; i <= p; i++) {
		r = cor[i];
		for (int j = 1; j < i; j++) r += alf[j] * cor[i - j];
		alf[i] = -(ref[i] = (r /= resid));
		int j = 0, k = i;
		while (++j <= --k) {
			a      = alf[j];
			alf[j] -= r * alf[k];
			if (j < k) alf[k] -= r * a;
		}
		resid *= (T(1) - r) * (T(1) + r);
	}
}

// 畳み込み
template<typename T>
inline void convol(T* u, int m, T* v, T* w, int n) {
	for (int i = 0; i < n; i++) {
		T sum = T(0);
		for (int j = 0; j < std::min(i + 1, m); j++) sum += u[j] * v[i - j];
		w[i] = sum;
	}
}

// 自己相関係数計算
template<typename T>
inline void sigcor(T* sig, int n, T* cor, int p) {
	T sqsum = T(0);
	for (int i = 0; i < n; i++) sqsum += sig[i] * sig[i];
	for (int k = 1; k < p + 1; k++) {
		T c = T(0);
		for (int i = k; i < n; i++) c += sig[i - k] * sig[i];
		cor[k] = c / sqsum;
	}
}

// 信号から PARCOR 係数を推定（sigcor + corref + convol の合成）
template<typename T>
inline void SigRef2(T* signal1, int n, int p, T* ref) {
	std::vector<T> signal2(n), cor(p + 1), alf(p + 1);
	sigcor(signal1, n, cor.data(), p);
	corref(p, cor.data(), alf.data(), ref);
	alf[0] = T(1);
	convol(alf.data(), p + 1, signal1, signal2.data(), n);
	std::copy_n(signal2.begin(), n, signal1);
}
