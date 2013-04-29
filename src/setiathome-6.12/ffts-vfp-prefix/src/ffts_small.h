#ifndef __FFTS_SMALL_H__
#define __FFTS_SMALL_H__


void vfp_firstpass_16_f(vfp_ffts_plan_t *  p, const void *  in, void *  out);
void vfp_firstpass_16_b(vfp_ffts_plan_t *  p, const void *  in, void *  out);
void vfp_firstpass_8_f(vfp_ffts_plan_t *  p, const void *  in, void *  out);
void vfp_firstpass_8_b(vfp_ffts_plan_t *  p, const void *  in, void *  out);
void vfp_firstpass_4_f(vfp_ffts_plan_t *  p, const void *  in, void *  out);
void vfp_firstpass_4_b(vfp_ffts_plan_t *  p, const void *  in, void *  out);
void vfp_firstpass_2(vfp_ffts_plan_t *  p, const void *  in, void *  out);

#endif
