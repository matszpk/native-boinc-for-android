/*
Copyright 2008-2010 Travis Desell, Dave Przybylo, Nathan Cole, Matthew
Arsenault, Boleslaw Szymanski, Heidi Newberg, Carlos Varela, Malik
Magdon-Ismail and Rensselaer Polytechnic Institute.

This file is part of Milkway@Home.

Milkyway@Home is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Milkyway@Home is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have reciveed a copy of the GNU General Public License
along with Milkyway@Home.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <time.h>

#ifdef ANDROID
#include "arm_math/fp2intfp.h"
#endif
#include "separation_types.h"
#include "likelihood.h"
#include "probabilities.h"
#include "integrals.h"
#include "r_points.h"
#include "calculated_constants.h"
#include "milkyway_util.h"
#include "separation_utils.h"
#include "evaluation_state.h"

/* CHECKME: What is this? */
static real probability_log(real bg, real sum_exp_weights)
{
    return mw_cmpzero_eps(bg, SEPARATION_EPS) ? -238.0 : mw_log10(bg / sum_exp_weights);
}

static const int calculateSeparation = 1;
static const int twoPanel = 1;

/* get stream & background weight constants */
static real get_stream_bg_weight_consts(StreamStats* ss, const Streams* streams)
{
    unsigned int i;
    real epsilon_b;
    real denom = 1.0;

    for (i = 0; i < streams->number_streams; i++)
        denom += mw_exp(streams->parameters[i].epsilon);

    for (i = 0; i < streams->number_streams; i++)
    {
        ss[i].epsilon_s = mw_exp(streams->parameters[i].epsilon) / denom;
        printf("epsilon_s[%d]: %lf\n", i, ss[i].epsilon_s);
    }

    epsilon_b = 1.0 / denom;
    printf("epsilon_b:    %lf\n", epsilon_b);
    return epsilon_b;
}

static void twoPanelSeparation(const AstronomyParameters* ap,
                               const SeparationResults* results,
                               StreamStats* ss,
                               const real* st_probs,
                               real bg_prob,
                               real epsilon_b)
{
    unsigned int i;
    real pbx, psgSum;

    pbx = epsilon_b * bg_prob / results->backgroundIntegral;

    for (i = 0; i < ap->number_streams; i++)
        ss[i].psg = ss[i].epsilon_s * st_probs[i] / results->streamIntegrals[i];

    psgSum = 0;
    for (i = 0; i < ap->number_streams; i++)
        psgSum += ss[i].psg;

    for (i = 0; i < ap->number_streams; i++)
        ss[i].sprob = ss[i].psg / (psgSum + pbx);

    for (i = 0; i < ap->number_streams; i++)
        ss[i].nstars += ss[i].sprob;
}

static void nonTwoPanelSeparation(StreamStats* ss, unsigned int number_streams)
{
    unsigned int i;

    for (i = 0; i < number_streams; i++)
    {
        ss[i].sprob = 1.0;
        ss[i].nstars += 1.0;
    }
}

static void separation(FILE* f,
                       const AstronomyParameters* ap,
                       const SeparationResults* results,
                       const mwmatrix cmatrix,
                       StreamStats* ss,
                       const real* st_probs,
                       real bg_prob,
                       real epsilon_b,
                       mwvector current_star_point)
{
    int s_ok;
    mwvector starxyz;
    mwvector starxyzTransform;

    mwvector xsun = ZERO_VECTOR;
    X(xsun) = ap->m_sun_r0;

    if (twoPanel)
        twoPanelSeparation(ap, results, ss, st_probs, bg_prob, epsilon_b);
    else
        nonTwoPanelSeparation(ss, ap->number_streams);

    /* determine if star with sprob should be put into stream */
    s_ok = prob_ok(ss, ap->number_streams);
    if (s_ok >= 1)
        ss[s_ok-1].q++;

    starxyz = lbr2xyz(ap, current_star_point);
    starxyzTransform = transform_point(ap, starxyz, cmatrix, xsun);

    if (f)
    {
        fprintf(f,
                "%d %lf %lf %lf\n",
                s_ok,
                X(starxyzTransform), Y(starxyzTransform), Z(starxyzTransform));
    }
}

#ifdef RDEBUG
static void print_probdata(const AstronomyParameters* ap,
    const StreamConstants* sc, const real* RESTRICT sg_dx, const real* RESTRICT r_point,
    const real* RESTRICT qw_r3_N, LBTrig lbt, real gPrime, real reff_xr_rp3,
    real* RESTRICT streamTmps)
{
  unsigned int i;
  fprintf(stderr,"AstronomyParams:\n  aux_bg_profile=%d\n  number_streams=%u\n  convolve=%u\n"
         "  bg_a_b_c=%1.18e,%1.18e,%1.18e\n  m_sun_r0=%1.18e\n"
         "  sun_r0=%1.18e\n  q_inv=%1.18e\n  q_inv_sqr=%1.18e\n"
         "  r0=%1.18e\n  alpha=%1.18e\n  alpha_delta3=%1.18e\n  wedge=%d\n"
         "  sun_r0=%1.18e\n  q=%1.18e\n  coeff=%1.18e\n"
         "  tcp=%1.18e\n  num_integrals=%u\n  exp_bg_w=%1.18e\n",
         ap->aux_bg_profile, ap->number_streams, ap->convolve,
         ap->bg_a, ap->bg_b, ap->bg_c,ap->m_sun_r0,
         ap->sun_r0,ap->q_inv,ap->q_inv_sqr,
         ap->r0, ap->alpha, ap->alpha_delta3, ap->wedge, ap->sun_r0, ap->q, ap->coeff,
         ap->total_calc_probs, ap->number_integrals, ap->exp_background_weight);
  //
  fputs("StreamConstants:", stderr);
  for (i=0; i<ap->number_streams; i++)
    fprintf(stderr,"  %d:a=[%1.18e,%1.18e,%1.18e,%1.18e]\n    c=[%1.18e,%1.18e,%1.18e,%1.18e]\n"
        "    sigma_sq2_inv=%1.18e,large_sigma=%d\n",
        i,sc[i].a.x,sc[i].a.y,sc[i].a.z,sc[i].a.w,
        sc[i].c.x,sc[i].c.y,sc[i].c.z,sc[i].c.w,
        sc[i].sigma_sq2_inv,sc[i].large_sigma);
  fputs("r_point",stderr);
  for (i=0; i<ap->convolve; i++)
    fprintf(stderr,"  %d:%1.18e\n",i,r_point[i]);
  fputs("qw_r3_N",stderr);
  for (i=0; i<ap->convolve; i++)
    fprintf(stderr,"  %d:%1.18e\n",i,qw_r3_N[i]);
  fprintf(stderr,"lbt: lCosBCos=%1.18e,real lSinBCos=%1.18e,bSin=%1.18e\n",
         lbt.lCosBCos,lbt.lSinBCos,lbt.bSin);
  fprintf(stderr,"gPrime=%1.18e\n",gPrime);
  fprintf(stderr,"reff_xr_rp3=%1.18e\n",reff_xr_rp3);
  fputs("sg_dx",stderr);
  for(i=0;i<ap->convolve;i++)
    fprintf(stderr,"  %d:%1.18e\n",i,sg_dx[i]);
  fputs("streamTmps",stderr);
  for(i=0;i<ap->number_streams;i++)
    fprintf(stderr,"  %d:%1.18e\n",i,streamTmps[i]);
}


static void print_outprob(real out, real* RESTRICT streamTmps)
{
  int i;
  printf("out=%1.18e\n",out);
  for(i=0;i<2;i++)
    printf("  %d:%1.18e\n",i,streamTmps[i]);
  fflush(stdout);
}
static int xcount=0;
#endif

#ifdef TEST_PROB_VFP
static double tmpStreamVals[5];

static int compare_doubles_2(const char* funcname, double a1, double a2)
{
    int mantisa;
    double diff = fabs(a1-a2);
    double frac = frexp(fmin(fabs(a1),fabs(a2)),&mantisa);
    if (!finite(a1) || !finite(a2))
    {
        fprintf(stderr,"%s error:%1.18e,%1.18e,%1.18e\n",
                funcname,a1,a2,diff);
        return 1;
    }
    if (diff*ldexp(1.0,-mantisa+1)>=6.220446049250313e-16)
    {
        fprintf(stderr,"%s error:%1.18e,%1.18e,%1.18e\n",
                funcname,a1,a2,diff);
        return 1;
    }
    return 0;
}
#endif

static real likelihood_probability(const AstronomyParameters* ap,
                                   const StreamConstants* sc,
                                   const Streams* streams,

                                   const real* RESTRICT sg_dx,
                                   const real* RESTRICT r_points,
                                   const real* RESTRICT qw_r3_N,

                                   const LBTrig lbt,
                                   real gPrime,
                                   real reff_xr_rp3,
                                   const SeparationResults* results,
                                   EvaluationState* es,

                                   real* RESTRICT bgProb) /* Out argument for thing needed by separation */
{
    unsigned int i;
    real starProb, streamOnly;

    /* if q is 0, there is no probability */
    if (ap->q == 0.0)
    {
        es->bgTmp = -1.0;
    }
    else
    {
#ifdef TEST_PROB_VFP
        double out;
        int i=0;
        int isbad = 0;
        es->bgTmp = prob_arm_vfpv3(ap, sc, sg_dx, r_points, qw_r3_N, lbt, gPrime, reff_xr_rp3, es->streamTmps);
        out = prob_arm_vfp(ap, sc, sg_dx, r_points, qw_r3_N, lbt, gPrime, reff_xr_rp3, tmpStreamVals);
        
        for (i=0; i<ap->number_streams;i++)
            isbad = compare_doubles_2("bgTmp",es->streamTmps[i],tmpStreamVals[i]);
        
        if (compare_doubles_2("bgTmp",es->bgTmp,out) || isbad)
        {
            print_probdata(ap, sc, sg_dx, r_points, qw_r3_N, lbt, gPrime, reff_xr_rp3, es->streamTmps);
            mw_finish(1);
        }
#else
        es->bgTmp = ((ProbabilityFunc)probabilityFunc)(ap, sc, sg_dx, r_points, qw_r3_N, lbt, gPrime, reff_xr_rp3, es->streamTmps);
#endif
    }

    if (bgProb)
        *bgProb = es->bgTmp;

    es->bgTmp = (es->bgTmp / results->backgroundIntegral) * ap->exp_background_weight;

    starProb = es->bgTmp; /* bg only */
    for (i = 0; i < ap->number_streams; ++i)
    {
        streamOnly = es->streamTmps[i] / results->streamIntegrals[i] * streams->parameters[i].epsilonExp;
        starProb += streamOnly;
        streamOnly = probability_log(streamOnly, streams->sumExpWeights);
        KAHAN_ADD(es->streamSums[i], streamOnly);
    }
    starProb /= streams->sumExpWeights;

    es->bgTmp = probability_log(es->bgTmp, streams->sumExpWeights);
    KAHAN_ADD(es->bgSum, es->bgTmp);

    return starProb;
}

#ifdef ANDROID
static real likelihood_probability_intfp(const AstronomyParameters* ap,
                                   const StreamConstantsIntFp* sc,
                                   const Streams* streams,

                                   const IntFp* RESTRICT sg_dx,
                                   const IntFp* RESTRICT r_points,
                                   const IntFp* RESTRICT qw_r3_N,

                                   const LBTrigIntFp* lbt,
                                   real gPrime,
                                   real reff_xr_rp3,
                                   const SeparationResults* results,
                                   EvaluationState* es,

                                   real* RESTRICT bgProb) /* Out argument for thing needed by separation */
{
    unsigned int i;
    real starProb, streamOnly;

    /* if q is 0, there is no probability */
    if (ap->q == 0.0)
    {
        es->bgTmp = -1.0;
    }
    else
    {
        es->bgTmp = ((ProbabilityFuncIntFp)probabilityFunc)(ap, sc, sg_dx, r_points,
              qw_r3_N, lbt, gPrime, reff_xr_rp3, es->streamTmps, es->streamTmpsIntFp);
    }

    if (bgProb)
        *bgProb = es->bgTmp;

    es->bgTmp = (es->bgTmp / results->backgroundIntegral) * ap->exp_background_weight;

    starProb = es->bgTmp; /* bg only */
    for (i = 0; i < ap->number_streams; ++i)
    {
        streamOnly = es->streamTmps[i] / results->streamIntegrals[i] * streams->parameters[i].epsilonExp;
        starProb += streamOnly;
        streamOnly = probability_log(streamOnly, streams->sumExpWeights);
        KAHAN_ADD(es->streamSums[i], streamOnly);
    }
    starProb /= streams->sumExpWeights;

    es->bgTmp = probability_log(es->bgTmp, streams->sumExpWeights);
    KAHAN_ADD(es->bgSum, es->bgTmp);
    
    return starProb;
}
#endif

static real calculateLikelihood(const Kahan* ksum, unsigned int nStars, unsigned int badJacobians)
{
    real sum;

    /*  log10(x * 0.001) = log10(x) - 3.0 */
    sum = ksum->sum + ksum->correction;
    sum /= (nStars - badJacobians);
    sum -= 3.0;

    return sum;
}

static void calculateLikelihoods(SeparationResults* results,
                                 Kahan* prob,
                                 Kahan* bgOnly,
                                 Kahan* streamOnly,
                                 unsigned int nStars,
                                 unsigned int nStreams,
                                 unsigned badJacobians)
{
    unsigned int i;

    /* CHECKME: badJacobians supposed to only be for final? */
    results->backgroundLikelihood = calculateLikelihood(bgOnly, nStars, 0);
    for (i = 0; i < nStreams; i++)
    {
        results->streamLikelihoods[i] = calculateLikelihood(&streamOnly[i], nStars, 0);
    }

    results->likelihood = calculateLikelihood(prob, nStars, badJacobians);
    
#ifdef RDEBUG
    warn("totLLIntegral:%1.18e\n",results->backgroundLikelihood);
    for (i  = 0; i < nStreams; ++i)
        warn("%u:totllstreamIntegral:%1.18e\n",i,results->streamLikelihoods[i]);
#endif
}

/* separation init stuffs */
static void setSeparationConstants(const AstronomyParameters* ap,
                                   const SeparationResults* results,
                                   mwmatrix cmatrix)
{
    unsigned int i;
    mwvector dnormal = ZERO_VECTOR;
    const mwvector dortho = mw_vec(0.0, 0.0, 1.0);

    dnormal = stripe_normal(ap->wedge);
    get_transform(cmatrix, dnormal, dortho);

    printf("\nTransformation matrix:\n"
           "\t%lf %lf %lf\n"
           "\t%lf %lf %lf\n"
           "\t%lf %lf %lf\n",
           X(cmatrix[0]), Y(cmatrix[0]), Z(cmatrix[0]),
           X(cmatrix[1]), Y(cmatrix[1]), Z(cmatrix[1]),
           X(cmatrix[2]), Y(cmatrix[2]), Z(cmatrix[2]));

    printf("==============================================\n");
    printf("bint: %lf\n", results->backgroundIntegral);
    for (i = 0; i < ap->number_streams; i++)
        printf("sint[%d]: %lf\n", i, results->streamIntegrals[i]);
}

static void printSeparationStats(const StreamStats* ss,
                                 const unsigned int number_stars,
                                 const unsigned int number_streams)
{
    unsigned int i;
    real percent;

    printf("%d total stars\n", number_stars);
    for (i = 0; i < number_streams; ++i)
    {
        percent = 100.0 * (ss[i].nstars / (real) number_stars);
        printf("%lf in stream[%d] (%lf%%)\n", ss[i].nstars, i, percent);
    }

    for (i = 0; i < number_streams; ++i)
        printf("%d stars separated into stream\n", ss[i].q);
}

static int likelihood_sum(SeparationResults* results,
                          const AstronomyParameters* ap,
                          const StarPoints* sp,
                          const StreamConstants* sc,
                          const Streams* streams,
                          const StreamGauss sg,

                          EvaluationState* es,

                          real* RESTRICT r_points,
                          real* RESTRICT qw_r3_N,


                          const int do_separation,
                          StreamStats* ss,
                          FILE* f)
{
    Kahan prob = ZERO_KAHAN;

    unsigned int current_star_point;
    mwvector point;
    real star_prob;
    LB lb;
    LBTrig lbt;
    real reff_xr_rp3;
    RConsts rc = { 0.0, 0.0 };

    real bgProb = 0.0;
    real epsilon_b = 0.0;
    mwmatrix cmatrix;
    unsigned int num_zero = 0;
    unsigned int badJacobians = 0;  /* CHECKME: Seems like this never changes */

    if (do_separation)
    {
        setSeparationConstants(ap, results, cmatrix);
        epsilon_b = get_stream_bg_weight_consts(ss, streams);
    }

    for (current_star_point = 0; current_star_point < sp->number_stars; ++current_star_point)
    {
        point = sp->stars[current_star_point];
        rc.gPrime = calcG(Z(point));
        setSplitRPoints(ap, sg, ap->convolve, rc.gPrime, r_points, qw_r3_N);
        reff_xr_rp3 = calcReffXrRp3(Z(point), rc.gPrime);

        LB_L(lb) = L(point);
        LB_B(lb) = B(point);

        lbt = lb_trig(lb);

        star_prob = likelihood_probability(ap, sc, streams, sg.dx, r_points, qw_r3_N, lbt, rc.gPrime,
                                           reff_xr_rp3, results, es, &bgProb);

        if (mw_cmpnzero_muleps(star_prob, SEPARATION_EPS))
        {
            star_prob = mw_log10(star_prob);
            KAHAN_ADD(prob, star_prob);
        }
        else
        {
            ++num_zero;
            prob.sum -= 238.0;
        }

        if (do_separation)
            separation(f, ap, results, cmatrix, ss, es->streamTmps, bgProb, epsilon_b, point);
    }

    calculateLikelihoods(results, &prob, &es->bgSum, es->streamSums,
                         sp->number_stars, streams->number_streams, badJacobians);


    if (do_separation)
        printSeparationStats(ss, sp->number_stars, ap->number_streams);

    return 0;
}

#ifdef ANDROID
static int likelihood_sum_intfp(SeparationResults* results,
                          const AstronomyParameters* ap,
                          const StarPoints* sp,
                          const StreamConstantsIntFp* sc,
                          const Streams* streams,
                          const StreamGauss sg,

                          EvaluationState* es,

                          real* RESTRICT r_points,
                          real* RESTRICT qw_r3_N,

                          IntFp* RESTRICT r_points_intfp,
                          IntFp* RESTRICT qw_r3_N_intfp,

                          const int do_separation,
                          StreamStats* ss,
                          FILE* f)
{
    Kahan prob = ZERO_KAHAN;

    unsigned int current_star_point;
    mwvector point;
    real star_prob;
    LB lb;
    LBTrigIntFp lbt;
    real reff_xr_rp3;
    RConsts rc = { 0.0, 0.0 };

    real bgProb = 0.0;
    real epsilon_b = 0.0;
    mwmatrix cmatrix;
    unsigned int num_zero = 0;
    unsigned int badJacobians = 0;  /* CHECKME: Seems like this never changes */
    unsigned int convolve = ap->convolve;
    unsigned int i;

    if (do_separation)
    {
        setSeparationConstants(ap, results, cmatrix);
        epsilon_b = get_stream_bg_weight_consts(ss, streams);
    }
    
    for (current_star_point = 0; current_star_point < sp->number_stars; ++current_star_point)
    {
        point = sp->stars[current_star_point];
        rc.gPrime = calcG(Z(point));
        setSplitRPoints(ap, sg, ap->convolve, rc.gPrime, r_points, qw_r3_N);
        reff_xr_rp3 = calcReffXrRp3(Z(point), rc.gPrime);

        for (i = 0; i < convolve; i++)
        {
            fp_to_intfp(r_points[i],&r_points_intfp[i]);
            fp_to_intfp(qw_r3_N[i],&qw_r3_N_intfp[i]);
        }
        
        LB_L(lb) = L(point);
        LB_B(lb) = B(point);

        lbt = lb_trig_intfp(lb);

        star_prob = likelihood_probability_intfp(ap, sc, streams, sg.dx_intfp, r_points_intfp, qw_r3_N_intfp, &lbt, rc.gPrime,
                                           reff_xr_rp3, results, es, &bgProb);

        if (mw_cmpnzero_muleps(star_prob, SEPARATION_EPS))
        {
            star_prob = mw_log10(star_prob);
            KAHAN_ADD(prob, star_prob);
        }
        else
        {
            ++num_zero;
            prob.sum -= 238.0;
        }

        if (do_separation)
            separation(f, ap, results, cmatrix, ss, es->streamTmps, bgProb, epsilon_b, point);
    }

    calculateLikelihoods(results, &prob, &es->bgSum, es->streamSums,
                         sp->number_stars, streams->number_streams, badJacobians);


    if (do_separation)
        printSeparationStats(ss, sp->number_stars, ap->number_streams);

    return 0;
}
#endif

StreamStats* newStreamStats(const unsigned int number_streams)
{
    return (StreamStats*) mwCallocA(number_streams, sizeof(StreamStats));
}

int likelihood(SeparationResults* results,
               const AstronomyParameters* ap,
               const StarPoints* sp,
               const StreamConstants* sc,
               const Streams* streams,
               const StreamGauss sg,
               const int do_separation,
               const char* separation_outfile)
{
    real* r_points;
    real* qw_r3_N;
    EvaluationState* es;
    StreamStats* ss = NULL;
    FILE* f = NULL;

    int rc = 0;
    double t1, t2;

    if (do_separation)
    {
        f = mw_fopen(separation_outfile, "w");
        if (!f)
        {
            perror("Opening separation output file");
            return 1;
        }

        ss = newStreamStats(streams->number_streams);
    }

    /* New state for this sum */
    es = newEvaluationState(ap);

    r_points = (real*) mwMallocA(sizeof(real) * ap->convolve);
    qw_r3_N = (real*) mwMallocA(sizeof(real) * ap->convolve);

    t1 = mwGetTime();
    
    rc = likelihood_sum(results,
                        ap, sp, sc, streams,
                        sg,
                        es,
                        r_points,
                        qw_r3_N,
                        do_separation,
                        ss,
                        f);
    t2 = mwGetTime();
    warn("Likelihood time = %f s\n", t2 - t1);

    mwFreeA(r_points);
    mwFreeA(qw_r3_N);
    mwFreeA(ss);
    freeEvaluationState(es);

    if (f && fclose(f))
        perror("Closing separation output file");

    return rc;
}


#ifdef ANDROID
int likelihood_intfp(SeparationResults* results,
               const AstronomyParameters* ap,
               const StarPoints* sp,
               const StreamConstantsIntFp* sc,
               const Streams* streams,
               const StreamGauss sg,
               const int do_separation,
               const char* separation_outfile)
{
    real* r_points;
    real* qw_r3_N;
    EvaluationState* es;
    StreamStats* ss = NULL;
    FILE* f = NULL;
    
    IntFp* r_points_intfp;
    IntFp* qw_r3_N_intfp;
    
    int rc = 0;
    double t1, t2;

    if (do_separation)
    {
        f = mw_fopen(separation_outfile, "w");
        if (!f)
        {
            perror("Opening separation output file");
            return 1;
        }

        ss = newStreamStats(streams->number_streams);
    }

    /* New state for this sum */
    es = newEvaluationState(ap);

    r_points = (real*) mwMallocA(sizeof(real) * ap->convolve);
    qw_r3_N = (real*) mwMallocA(sizeof(real) * ap->convolve);
    r_points_intfp = (IntFp*)mwMallocA(sizeof(IntFp)*ap->convolve);
    qw_r3_N_intfp = (IntFp*)mwMallocA(sizeof(IntFp)*ap->convolve);
    
    t1 = mwGetTime();
    
    rc = likelihood_sum_intfp(results,
                                ap, sp, sc, streams,
                                sg,
                                es,
                                r_points,
                                qw_r3_N,
                                r_points_intfp,
                                qw_r3_N_intfp,
                                do_separation,
                                ss,
                                f);
    t2 = mwGetTime();
    warn("Likelihood time = %f s\n", t2 - t1);

    mwFreeA(r_points);
    mwFreeA(qw_r3_N);
    mwFreeA(r_points_intfp);
    mwFreeA(qw_r3_N_intfp);
    mwFreeA(ss);
    freeEvaluationState(es);

    if (f && fclose(f))
        perror("Closing separation output file");

    return rc;
}
#endif
