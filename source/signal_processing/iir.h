// TODO: Replace this filter coefficient computation implementation with a new one,
// and filter using the more efficient PowerQuad based biquad IIR filter.

#ifndef _IIR_H_
#define _IIR_H_

#ifdef __cplusplus
extern "C" {
#endif

double *binomial_mult( int n, double *p, double *a );
double *trinomial_mult( int n, double *b, double *c, double *a );

double *dcof_bwlp( int n, double fcf, double* rcof, double* dcof );
double *dcof_bwhp( int n, double fcf, double* rcof, double* dcof );
double *dcof_bwbp( int n, double f1f, double f2f, double* rcof, double* tcof, double* dcof );
double *dcof_bwbs( int n, double f1f, double f2f, double* rcof, double* tcof, double* dcof );

double *ccof_bwlp( int n, double* ccof );
double *ccof_bwhp( int n, double* ccof  );
double *ccof_bwbp( int n, double* tcof, double* ccof );
double *ccof_bwbs( int n, double f1f, double f2f, double* ccof );

double sf_bwlp( int n, double fcf );
double sf_bwhp( int n, double fcf );
double sf_bwbp( int n, double f1f, double f2f );
double sf_bwbs( int n, double f1f, double f2f );

#ifdef __cplusplus
}
#endif

#endif /* _IIR_H_ */

