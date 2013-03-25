#include <stdio.h>
#include <math.h>

class model {
public :
    double zen_corr_coeff[13];
    double az_corr_coeff[13];
};

void ZenAzCorrection(model& model, double zen, double az, double& zencorr, double& azcorr) {

  //for(int i=0; i<13; i++) printf("%lf\n", model.az_corr_coeff[i]);

  static const double D2R=(M_PI/180.0);  /* decimal degrees to radians conversion */

  double CosAz, SinAz, SinZen, Cos2Az, Sin2Az, Cos3Az, Sin3Az, Cos6Az, Sin6Az, SinZen2,
         Cos3Imb, Sin3Imb, AzRadians, ZenRadians, ZenCorrection, AzCorrection;

  // Start of port of AO Phil's IDL routine modeval.pro
  AzRadians  = az  * D2R;
  ZenRadians = zen * D2R;

  CosAz  = cos(AzRadians);
  SinAz  = sin(AzRadians);
  Cos2Az = cos(2.0 * AzRadians);
  Sin2Az = sin(2.0 * AzRadians);
  Cos3Az = cos(3.0 * AzRadians);
  Sin3Az = sin(3.0 * AzRadians);
  Cos6Az = cos(6.0 * AzRadians);
  Sin6Az = sin(6.0 * AzRadians);

  SinZen  = sin(ZenRadians);
  SinZen2 = SinZen * SinZen;

  Cos3Imb = sin(ZenRadians - 0.1596997627) * Cos3Az;	// 0.159... = 9.15 deg.  This is related
  Sin3Imb = sin(ZenRadians - 0.1596997627) * Sin3Az;    // to the balance of dome and CH. (via Phil)

  zencorr = model.zen_corr_coeff[ 0]         	+
                  model.zen_corr_coeff[ 1] * CosAz 	+
                  model.zen_corr_coeff[ 2] * SinAz 	+
                  model.zen_corr_coeff[ 3] * SinZen	+
		          model.zen_corr_coeff[ 4] * SinZen2	+	 
		          model.zen_corr_coeff[ 5] * Cos3Az	+
		          model.zen_corr_coeff[ 6] * Sin3Az	+
		          model.zen_corr_coeff[ 7] * Cos3Imb	+
		          model.zen_corr_coeff[ 8] * Sin3Imb	+
		          model.zen_corr_coeff[ 9] * Cos2Az	+
		          model.zen_corr_coeff[10] * Sin2Az	+
		          model.zen_corr_coeff[11] * Cos6Az	+
		          model.zen_corr_coeff[12] * Sin6Az;

  azcorr  = model.az_corr_coeff[ 0]                +
                  model.az_corr_coeff[ 1] * CosAz        +
                  model.az_corr_coeff[ 2] * SinAz        +
                  model.az_corr_coeff[ 3] * SinZen       +
                  model.az_corr_coeff[ 4] * SinZen2      +
                  model.az_corr_coeff[ 5] * Cos3Az       +
                  model.az_corr_coeff[ 6] * Sin3Az       +
                  model.az_corr_coeff[ 7] * Cos3Imb      +
                  model.az_corr_coeff[ 8] * Sin3Imb      +
                  model.az_corr_coeff[ 9] * Cos2Az       +
                  model.az_corr_coeff[10] * Sin2Az       +
                  model.az_corr_coeff[11] * Cos6Az       +
                  model.az_corr_coeff[12] * Sin6Az;
// end of port of modeval.pro

  zencorr = zencorr / 3600;
  azcorr  = azcorr / 3600;
}

int main() {

double zencorr_1, azcorr_1;
model model_1;
// these are our current alfa model coefficients as of 06/05/2012 (same as 4/20/2011)
model_1.zen_corr_coeff = {-57.5499992371,-95.5599975586,-4.13000011444,141.6900024414,
                          677.5100097656,-10.4099998474,-7.71000003815,-10.3900003433,
                          0.079999998212,0.430000007153,-0.62000000477,0.029999999329,
                          -0.36000001431};
model_1.az_corr_coeff  = {-37.0000000000,-6.05000019073,92.34999847412,-731.210021973,
                          -1013.96997070,-24.5300006866,-11.1899995804,9.180000305176,
                          106.0400009155,3.019999980926,-1.74000000954,-3.46000003815,
                          1.289999961853};

double zencorr_2, azcorr_2;
model model_2;
// these are the newest alfa model coefficients, from Phil, as of 06/05/2012 
model_2.zen_corr_coeff = {-64.85, -97.13, -1.82, 132.62,   
                          721.35, -10.25, -6.13, -10.92,   
                          11.58, 1.05, -1.70, -0.26,    
                         -0.12};     
model_2.az_corr_coeff  = {-34.73, -3.14, 93.49, -715.60,   
                          -1089.56, -21.52, -10.85, 18.25,   
                          111.09, 7.92, 0.13, -2.56,    
                          0.56};     

// these are the alfa model coefficients, from Phil, as of 4/20/2011
//model_2.zen_corr_coeff = {-57.92,-86.12,2.01,125.02,  
//                            713.55,-12.12,-5.89,-5.13,  
//                            6.49,0.48,-0.84,0.09,  
//                            0.15};
//model_2.az_corr_coeff  = {-27.98,0.73,83.10,-776.49,
//                            -960.35,-20.07,-9.17,17.95, 
//                            115.98,6.35,0.02,-2.16, 
//                            1.67};  

const int zen_point_size=5;
double zen_point[zen_point_size] = {0,5,10,15,20};
const int az_point_size=6;
double az_point[az_point_size]  = {0,45,90,135,180,215};

for(int i=0; i<zen_point_size; i++) {
    for(int j=0; j<az_point_size; j++) {
        ZenAzCorrection(model_1, zen_point[i], az_point[j], zencorr_1, azcorr_1);
        ZenAzCorrection(model_2, zen_point[i], az_point[j], zencorr_2, azcorr_2);
        fprintf(stdout, "zen = %lf az = %lf zen diff = %lf arcsec az diff = %lf arcsec\n", 
                zen_point[i], az_point[j], fabs(zencorr_1-zencorr_2)*3600, fabs(azcorr_1-azcorr_2)*3600);  
    }
    fprintf(stdout, "\n");
}
}

