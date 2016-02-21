
#pragma once

#include<vector>

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




double refsig(const int p, const double* ref,double* state, const double xin);
void corref(int p,double* cor,double* alf,double* ref);
void convol( double* u, int m, double* v, double* w, int n );
void sigcor(double* sig,int n,double* cor,int p);
void SigRef2(double* signal1, int n, int p, double* ref);

