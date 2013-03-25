#include "../config.h"
#include "simd.h"     
typedef float sah_complex[2];

sah_complex FreqData[512] __attribute__ ((aligned(16)))={{ 0,1},{2,3},{4,5},{6,7},{8,9},{1,1},{.70710678118654752440,.70710678118654752440}} ;
float PowerSpectrum[512] __attribute__ ((aligned(16)));


int main(void) {
     register simd<float,4>::ptr p3(PowerSpectrum);
register __m128 xmm0 asm("xmm0");
register __m128 xmm1 asm("xmm1");
register __m128 xmm2 asm("xmm2");
register __m128 xmm3 asm("xmm3");
register __m128 xmm4 asm("xmm4");
register __m128 xmm5 asm("xmm5");
register __m128 xmm6 asm("xmm6");
register __m128 xmm7 asm("xmm7");
     for (unsigned int i=0;i<512;i+=4) {
     /*
       register simd<float,4>::ptr p1(FreqData+i*2),p2(FreqData+i*2+4);
       f1=(*p1*(*p1))+shuffle<0x1032>(*p1*(*p1));
       f2=(*p2*(*p2))+shuffle<0x1032>(*p2*(*p2));
       p3[i]=interleave<0x0202>(f1,f2);
       */
     __asm__ ( 
        "mulps %0,%0\n\t"
        "mulps %4,%4\n\t"
        "movaps %0,%1\n\t"
        "movaps %4,%2\n\t"
        "shufps $177,%0,%0\n\t"
        "shufps $177,%4,%4\n\t"
        "addps %1,%0\n\t"
        "addps %2,%4\n\t"
        "shufps $136,%4,%0\n\t"
        : "=x" (*(__m128 *)(&(PowerSpectrum[i]))), "=x" (xmm6), "=x" (xmm7)
        : "0" (*(__m128 *)(&(FreqData[i]))), "x" (*(__m128 *)(&(FreqData[i+2])))
        ) ;
/*
 *        __asm__ ( 
       	"mulps %0,%0\n\t"
	"mulps %4,%4\n\t"
	"movaps %0,%1\n\t"
	"movaps %4,%2\n\t"
	"shufps $177,%0,%0\n\t"
	"shufps $177,%4,%4\n\t"
	"addps %1,%0\n\t"
	"addps %2,%4\n\t"
	"shufps $136,%4,%0\n\t"
	: "=x" (*(__m128 *)(PowerSpectrum+i)),"=x" (xmm6),"=x" (xmm7)
	: "0" (*(__m128 *)(FreqData+i*2)), "x" (*(__m128 *)(FreqData+i*2+4))
	) ;
*/
     }
     simd<float,4>::simd_mode_finish();
     printf("%f %f %f %f %f %f %f %f\n",PowerSpectrum[0],PowerSpectrum[1],PowerSpectrum[2],PowerSpectrum[3],PowerSpectrum[4],PowerSpectrum[5],PowerSpectrum[6],PowerSpectrum[7]);
     return(0);
}

