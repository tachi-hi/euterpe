/* --- refsig ---

******************************************************************
*     LPC / PARCOR / CSM / LSP   subroutine  library     # 51    *
*               coded by S.Sagayama,                4/23/1981    *
******************************************************************
 ( C version coded by S.Sagayama, 10/??/1986 )

   description:
     * conversion of "ref" into "sig".
     * speech synthesizer filter whose coefficients are "ref".
     * PARCOR all-pole filter.

   synopsis:
             ------------------------
             refsig(ip,cep,state,xin)
             ------------------------
   refsig  : output.       double function.
             sample output of PARCOR all-pole filter.
   p       : input.        integer.
             the order of analysis; the number of poles in LPC;
             the degree of freedom of the model - 1.
   ref[.]  : input.        double array : dimension=p+1.
             PARCOR coefficients; reflection coefficients.
             all of ref[.] range between -1 and 1.
   state[.]: input/output. double array : dimension=p.
             inner state in the PARCOR filter. let be all 0
             for initialization.
   xin     : input.        double.
             filter input sample.


整理 橘
*/

#include"signalProcessingLibrary.h"
#include<algorithm>

void PARCOR::init(int p_){
	p = p_;
	state.resize(p);
}

// PARCOR all pole filter?
double PARCOR::refsig(const double* ref, const double xin){
  double y = xin + ref[p] * state[p - 1];
  for(int i = p - 1; i > 0; i--) { 
    y += ref[i] * state[i - 1]; 
    state[i] = state[i - 1] - ref[i] * y; 
  }
  state[0] = y; 
  return(y);
}

//２つの対象配列のコンボリューション？
void PARCOR::convol(const double* u, const int m, const double* v, double* w, const int n ){
  for(int i = 0; i < n; i++) {
    double sum = 0.;
    for(int j = 0; j < std::min(i + 1, m); j++) {
      sum += u[j] * v[i - j];
    }
		w[i] = sum;
  }
}




void SigRef2(double* signal1, int n, int p, double* ref){
  double *signal2,*cor,*alf;
  int i;
  signal2 = new double[n];
  cor     = new double[p+1];
  alf     = new double[p+1];
  
  sigcor(signal1, n, cor, p);
  corref(p, cor, alf, ref);
  alf[0] = 1.0;
  convol(alf, p+1, signal1, signal2, n);
  for(i = 0; i < n; i++){
    signal1[i] = signal2[i];
  }
	delete[] signal2;
	delete[] cor;
	delete[] alf;
}


// PARCOR all pole filter?
double refsig(const int p, const double* ref, double* state, const double xin){
  if( p <= 0 ){ // error check?
    return(xin);
  }
  double y = xin + ref[p] * state[p - 1];
  for(int i = p - 1; i > 0; i--) { 
    y += ref[i] * state[i - 1]; 
    state[i] = state[i - 1] - ref[i] * y; 
  }
  state[0] = y; 
  return(y);
}


/*--- corref ---

******************************************************************
*     LPC / PARCOR / CSM / LSP   subroutine  library     # 01    *
*               coded by S.Sagayama,           september/1976    *
******************************************************************
 ( C version coded by S.Sagayama; revised,  6/20/1986, 2/4/1987 )

 - description:
   * conversion of "cor" into "ref".
     "" is simultaneously obtained.
   * computation of PARCOR coefficients "ref" of an arbitrary
     signal from its autocorrelation "cor".
   * computation of orthogonal polynomial coefficients from 
     autocorrelation function.
   * recursive algorithm for solving toeplitz matrix equation.
     example(p=3):  solve in respect to a1, a2, and a3.
         ( v0 v1 v2 )   ( a1 )     ( v1 )
         ( v1 v0 v1 ) * ( a2 ) = - ( v2 )
         ( v2 v1 v0 )   ( a3 )     ( v3 )
     where v0 = 1, vj = cor(j), aj = (j).
   * recursive computation of coefficients of a polynomial:(ex,p=4)
               | v0   v1   v2   v3   |    /      | v0   v1   v2 |
     A(z)= det | v1   v0   v1   v2   |   /   det | v1   v0   v1 |
               | v2   v1   v0   v1   |  /        | v2   v1   v0 |
               | 1    z   z**2 z**3  | /       
     where A(z) = z**p + (1) * z**(p-1) + ... + (p).
     note that the coefficient of z**3 is always equal to 1.
   * Gram-Schmidt orthogonalization of a sequence, ( 1, z, z**2,
     z**3, ... ,z**(2n-1) ), on the unit circle, giving their inner
     products:
                       k    l
          v(k-l) = ( z  , z   ),    0 =< k,l =< p.
     where v(j) = cor(j), v(0) = 1.
     coefficients of p-th order orthogonal polynomial are obtained
     through this subroutine. ((1),...,(p))
   * computation of reflection coefficients ref(i) at the boundary
     of the i-th section and (i+1)-th section in acoustic tube
     modeling of vocal tract.
   * the necesary and sufficient condition for existence of
     solution is that toeplitz matrix ( v(i-j) ), i,j=0,1,... 
     be positive definite.

 - synopsis:
          ----------------------------
          corref(p,cor,alf,ref,&resid)
          ----------------------------
   p       : input.        integer.
             the order of analysis; the number of poles in LPC;
             the degree of freedom of the model - 1.
   cor[.]  : input.        double array : dimension=p+1
             autocorrelation coefficients.
             cor[0] is implicitly assumed to be 1.0.
   alf[.]  : output.       double array : dimension=p+1
             linear prediction coefficients; AR parameters.
             [0] is implicitly assumed to be 1.0.
   ref[.]  : output.       double array : dimension=p+1
             PARCOR coefficients; reflection coefficients.
             all of ref[.] range between -1 and 1.
   resid   : output.       double.
             linear prediction / PARCOR residual power;
             reciprocal of power gain of PARCOR/LPC/LSP all-pole filter.

 - note: * if p<0, p is regarded as p=0. then, resid=1, and
           alf[.] and ref[.] are not obtained.

 - fortran equivalence:
*/

/* shorter version -- This requires 1 more division than former version */
/*
corref(p,cor,alf,ref,_resid) int p; double cor[],alf[],ref[],*_resid;
{ double resid,r,a; int i,j,k;
  resid=1.0;
  for(i=1;i<=p;i++)
  { r=cor[i]; for(j=1;j<i;j++) r+=alf[j]*cor[i-j];
    alf[i]= -(ref[i]=(r/=resid));
    j=0; k=i;
    while(++j<=--k) { a=alf[j]; alf[j]-=r*alf[k]; if(j<k) alf[k]-=r*a; }
    resid*=(1.0-r)*(1.0+r); } }
  *_resid=resid; }
*/



void PARCOR::corref(double* cor,double* alf,double* ref){
//  double cor[],   /* in: correlation coefficients */
//         alf[],   /* out: linear predictive coefficients */
//         ref[],   /* out: reflection coefficients */
//         *resid_; /* out: normalized residual power */
  ref[1] = cor[1];
  alf[1] = -ref[1];
  double resid=(1.0-ref[1])*(1.0+ref[1]);
  for(int i = 2 ;i <= p; i++){
    double r = cor[i];
    for(int j = 1; j < i; j++){
      r += alf[j]*cor[i-j];
    }
    alf[i]= -(ref[i]=(r/=resid));

    int j = 0; int k = i;
    while(++j<=--k){ 
      double a = alf[j];
      alf[j] -= r * alf[k];
      if(j < k) { 
        alf[k] -= r * a;
      }
    }
   resid *= (1.0 - r) * (1.0 + r);
  }
}

void corref(int p,double* cor,double* alf,double* ref){
  int i,j,k;
  double resid,r,a;
  ref[1]=cor[1];
  alf[1]= -ref[1];
  resid=(1.0-ref[1])*(1.0+ref[1]);
  for(i=2;i<=p;i++){
    r=cor[i];
    for(j=1;j<i;j++){
      r+=alf[j]*cor[i-j];
    }
    alf[i]= -(ref[i]=(r/=resid));
    j=0; 
    k=i;
    while(++j<=--k){ 
      a=alf[j];
      alf[j]-=r*alf[k];
      if(j<k) { 
        alf[k]-=r*a;
      }
    }
    resid*=(1.0-r)*(1.0+r);
  }
}

/*
 --- convol ---

******************************************************************
*     lpc / parcor / csm / lsp   subroutine  library     # nn    *
*               coded by s.sagayama,               mm/dd/19yy    *
******************************************************************

   description:
     * convolution of two symmetrical arrays.
        u*v=w, * = convolution, where u and v are symmetrical
       arrays such as correlation functions. convolved result
       w is also a symmetrical array.
       for eaxmple:
        u -- impulse response autocorrelation of a linear filter.
        v -- input signal autocorrelation.
        w -- output signal autocorrelation.

   synopsis:
          -----------------------
          void convol(u,m,v,w,n)
          -----------------------
   u(.)    : input.        double array : dimension=m.
             elements of a symmetrical array,
              u(0),u(1),u(2),...,u(m-1),
             where u(i)=u(-i) for i=1,2,...,m-1.
   m       : input.        integer.
             the dimension of the array u(.).
   v(.)    : input.        double array : dimension=m+n.
             elements of a symmetrical array,
              v(0),v(1),v(2),...,v(m+n-1),
             where v(i)=(v-i) for i=1,2,...,m+n-1.
   w(.)    : output.       double array : dimension=n.
             elements of a symmetrical array,
              w(0),w(1),w(2),...,w(n-1),
             where w(i)=w(-i) for i=1,2,...,n-1.
   n       : input.        integer.
             the dimension of the array w(.).
*/


void convol( double* u, int m, double* v, double* w, int n ){
  for(int i = 0; i < n; i++) {
    double sum = 0.;
    for(int j = 0; j < std::min(i + 1, m); j++) {
      sum += u[j] * v[i - j];
    }
		w[i] = sum;
  }

/*
  for(int i = 0; i < m; i++) {
    w[i] = 0;
    for(int j = 0; j <= i; j++) {
      w[i] += u[j] * v[i-j];
    }
  }
  for(int i = m; i < n; i++) {
    w[i] = 0;
    for(int j = 0; j < m; j++) {
      w[i] += u[j] * v[i-j];
    }
  }
*/
}

/* --- sigcor ---

******************************************************************
*     LPC / PARCOR / CSM / LSP   subroutine  library     # 50    *
*               coded by S.Sagayama,                7/25/1979    *
******************************************************************
 ( C version coded by S.Sagayama, 2/05/1987; 20 Feb 1997 )
 ( revised by S.Sagayama, 23 June 1997 )

   description:
     * conversion of "sig" into "cor".
     * computation of autocorrelation coefficients "cor" from
       signal samples.

   synopsis:
          ------------------------
          sigcor(sig,n,&pow,cor,p)
          ------------------------
   n       : input.        integer.
             length of sample sequence.
   sig[.]  : input.        double array : dimension=n. 
             signal sample sequence.
   pow     : output.       double.
             power. (average energy per sample).
   cor[.]  : output.       double array : dimension=lmax
             autocorrelation coefficients.
             cor[0] is implicitly assumed to be 1.0.
   p       : input.        integer.
             the number of autocorrelation points required.
*/

void sigcor(double* sig,int n,double* cor,int p){
  double sqsum=0.0;
	for(int i = 0; i < n; i++) { 
		sqsum += sig[i] * sig[i];
	}
	for(int k = 1; k < p + 1; k++) {
		double c = 0.0; 
		for(int i = k; i < n; i++){ 
			c += sig[i - k] * sig[i]; 
		}
		cor[k] = c/sqsum;
	}
}

