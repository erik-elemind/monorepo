/*
 * dwt97level.c
 *
 *  Created on: Mar 13, 2022
 *      Author: DavidWang
 */

#include <math.h>
#include "dwt97.h"

// *  The first half part of the output signal contains the approximation coefficients.
// *  The second half part contains the detail coefficients (aka. the wavelets coefficients).
//N1 = size(X,1);
//N2 = size(X,2);

void fwt97level(CMPR_FLOAT* x,int n,int level){
// FROM MATLAB:
//  for k = 1:Level // range is inclusive
//     M1 = ceil(N1/2);
//     M2 = ceil(N2/2);
//     // fwt97()
//     N1 = M1;
//     N2 = M2;
//  end

  for (int k=1; k<=level; k++){
    int m1 = ceil(n/2);
    CMPR_LOG_PUTI(m1);
    CMPR_LOG_PUTS("\n");
    if(n>1){
      fwt97(x,n);
    }
    n = m1;
  }
}

void iwt97level(CMPR_FLOAT* x,int n,int level){
// FROM MATLAB:
//  for k = 1+Level:0 // range is inclusive
//     M1 = ceil(N1*pow2(k));
//     M2 = ceil(N2*pow2(k));
//     // iwt97()
//  end
  level = -abs(level);
  for(int k=1+level; k<=0; k++){
    int m1 = ceil(n*pow(2,k));
    if (m1 > 1){
      iwt97(x,m1);
    }
  }
}


