#include "setilib.h"

void seti_hanning_window(float hanning_window[], int num_points) {
  int i;

  float two_pi    = M_PI * 2.0;
  float N_minus_1 = num_points - 1.0;

  for(i=0; i<num_points; i++) {
	hanning_window[i] = 0.5 * ( 1.0 - cosf((two_pi*i)/N_minus_1) );
  }
}
