#include "../config.h"
#include "simd.h"     

void v_GetPowerSpectrum(
    float* FreqData,
    float* PowerSpectrum,
    int NumDataPoints
) {
    int i, j;

    simd<float,4>::ptr spec(PowerSpectrum),data(FreqData);

    for (i=0,j=0; i < (NumDataPoints*2)/simd<float,4>::size; i+=2,j++) {
        spec[j].prefetchw();
	data[i].prefetch();
	spec[j]=interleave<0x0202>(
	     data[i]*data[i]+shuffle<0x1032>(data[i]*data[i]),
	     data[i+1]*data[i+1]+shuffle<0x1032>(data[i+1]*data[i+1])
	     );

    }
}

