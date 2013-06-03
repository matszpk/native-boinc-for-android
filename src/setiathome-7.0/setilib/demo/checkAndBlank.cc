// * * * * * * * * * * * * * * * * * * *
//
// Check and Blank || Mark 4
//
// Luke Zoltan Kelley
// Created: 062508
// Updated: 091108
//
// 
// adapt thresholds and settings to results - esp not enough locked
// numerous signals alternating - have lock list include, and generaterBS effect
// numerous signals overlapping?
// generalize to any number of and type of radar signal
// allow skips in locking on
//
// * * * * * * * * * * *

#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "setilib.h"
#include <vector>
#include <complex>
#include <string>
#include <iostream>
#include <fstream>
#include <time.h>

int fileD, channel, sample;
int retVal,retVal2,retVal3,retVal4;
char * dFile;
const int numSampD = 62500000;     //25 seconds
//const int numSampD = 30000000;     //12 s
//const int numSampD = 15000000;     //6 s
//const int numSampD = 2500000;    //1 s
const int numChanD = 16;  //16 vs 14
const int binSize = 10;
const int thresh = 22;                   //threshold for sum of all channels to count as signal
const int stop = 1;                      //0 = complete file; # = number of iterations (iter) to procede through
const int printOldBS = 1;
const int printNewBS = 1;
const int printRS = 1;


const int expDur[5][7] = {{ 0 , 0 , 0 , 0 , 0 , 0 , 0 },
			  { 6998 , 0 , 0 , 0 , 0 , 0 , 0 },
			  { 6781 , 0 , 0 , 0 , 0 , 0 , 0 },
			  { 6581 , 7052 , 6864 , 6487 , 8274 , 0 , 0 },
			  { 8759 , 7688 , 7021 , 7260 , 8224 , 9189 , 9428 }};

const int numPats = 5;
const int numAlts = 7;

const int slack = 20;                    //exceptable range around low durations to still count
const int blankLength = 800;
const int reJustLength = 500;            //will search this distance on each side of current BS start for a RS peak to readjust to
const double relStartLoc = 0.15;
const int igLow = 70;

const int length12L = 29500000;
const int length12H = 30250000;

int actDur[5][7] = {{ 0 , 0 , 0 , 0 , 0 , 0 , 0 },
		    { 0 , 0 , 0 , 0 , 0 , 0 , 0 },
		    { 0 , 0 , 0 , 0 , 0 , 0 , 0 },
		    { 0 , 0 , 0 , 0 , 0 , 0 , 0 },
		    { 0 , 0 , 0 , 0 , 0 , 0 , 0 }};

int numSamp, numChan, isBS, isRS;
long double percentComp = 0.0;
char oFileName[40];
char oFileName1[40]; 
char oFileName2[40];
char oFileName3[40];
char oFileName4[40];
std::vector<complex<int> > cw;
std::ofstream oFileOBS, oFile, oFileInfo, oFileNBS, oFileDUR;
dataheader_t dataHead;

struct Signal {
  long int loc; //rel. location in samples
  int strength; //ave strength of signal
  long int dur; //low duration since last signal
  int type;     //if matches radar - type of radar
  int locked;   //if sequence of radar - locked on (0||1)
};

// * * Primary Functions * *
int sumSignal( std::vector<complex<int> > * dataIn , std::vector<Signal> &sigOut , int length , int iteration);
int findBlanking( std::vector<int> inBlank, int iBSHigh, int iBSLow );
int findRadar( std::vector<Signal> & sigsIn , int *patsOut);
double findLocks( std::vector<Signal> & sigsIn , long double *patFreq , unsigned long long int &oIndex , int numSamp); 
void generateBS( std::vector<Signal> sigsIn , std::vector<int> & genBlank , unsigned long int startIndex , unsigned long long int startLoc , int numSamp );

// * * Secondary Functions * *
bool compDur(int dur1, int dur2, int slack);    //returns t/f whether dur1 is within dur2 by +- slack
int getBigType(int smallType);                  //based on total pat type (i.e. 23 || 34) return large pat type (i.e. 0-numPats)
int getSmallType(int nowType);                  //  "    "    "   "   "                      "    small "   "   (i.e. 0-6)
int getNextType(int nowType);                   //based on current small pat type, return next small type
int getLastType(int nowType);                   //     "       "         "       , return last small type
long int getNextDur(int nowType);               //returns the next low durations based on current small type
long int getNext2Dur(int nowType);

//FIX pass this to method instead of being global
std::vector<int> newPat;

int main(int argc, char ** argv) {
  printf("\nCheck and Blank Signal Started. \n\n");

  //Parse Command Line
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-f")) { dFile = argv[++i]; }
    else {
      fprintf(stderr, "usage : blankSignal -f filename\n");
      exit(1);
    }
  }
  
  if (dFile == NULL) { 
    fprintf(stderr, "usage : blankSignal -f filename\n");
    exit(1);
  }
  
  numSamp = numSampD;

  std::vector<complex<int> > data[numChanD];    //array of complex vectors, array size = numSamp; ea vector size = numChan
  std::vector<complex<int> > compBlankSig;
  std::vector<int> blankSig;                    //stores existing blanking signal from fata file
  std::vector<Signal> sigs;                     //detected pulses and information regarding
  std::vector<unsigned long long> chanLoc[2];
  
  
  // * * * * * * * * GET DATA * * * * * * * *
  
  //Initialize Data Files
  fileD = open(dFile, O_RDONLY);
  if(fileD == -1) {
    printf("  Error opening data file. Exitting...\n ");
    exit(1);
  } else { printf("     Data file opened:  "); }
  printf("%s \n", dFile);
  
  strncat(oFileName,dFile,15);
  strcat(oFileName,"_RS.txt");
  if(remove("RS_backup.dat")) {}
  if(rename(oFileName, "RS_backup.dat")) {}
  oFile.open(oFileName);
  if(oFile.is_open() && oFile.good()) { printf("  Output file created:  "); }
  else {
    printf("  Error creating output file...Exiting\n");
    oFile.close();
    exit(1);
  }
  printf("%s \n" , oFileName);

  strncat(oFileName1,dFile,15);
  strcat(oFileName1,"_Info.txt");
  oFileInfo.open(oFileName1);
  if(oFileInfo.is_open() && oFileInfo.good()) { printf("  Output file created:  "); }
  else {
    printf("  Error creating output file...Exiting\n");
    oFile.close();
    exit(1);
  }
  printf("%s \n" , oFileName1);

  if(printOldBS == 1) {
    strncat(oFileName2,dFile,15);
    strcat(oFileName2,"_oBS.txt");
    if(remove("oBS_backup.dat")) {}
    if(rename(oFileName2, "oBS_backup.dat")) {}
    oFileOBS.open(oFileName2);
    if(oFileOBS.is_open() && oFileOBS.good()) { printf("  Output file created:  "); }
    else {
      printf("  Error creating output file...Exiting\n");
      oFile.close();
      exit(1);
    }
    printf("%s \n" , oFileName2);
  }

  if(printNewBS == 1) {
    strncat(oFileName3,dFile,15);
    strcat(oFileName3,"_nBS.txt");
    if(remove("nBS_backup.dat")) {}
    if(rename(oFileName3, "nBS_backup.dat")) {}
    oFileNBS.open(oFileName3);
    if(oFileNBS.is_open() && oFileNBS.good()) { printf("  Output file created:  "); }
    else {
      printf("  Error creating output file...Exiting\n");
      oFileNBS.close();
      exit(1);
    }
    printf("%s \n" , oFileName3);  
  }
  
  if(printRS == 1) { 
    strncat(oFileName4,dFile,15);
    strcat(oFileName4,"_Dur.txt");
    if(remove("Dur_backup.dat")) {}
    if(rename(oFileName4, "Dur_backup.dat")) {}
    oFileDUR.open(oFileName4);
    if(oFileDUR.is_open() && oFileDUR.good()) { printf("  Output file created:  "); }
    else {
      printf("  Error creating output file...Exiting\n");
      oFileDUR.close();
      exit(1);
    }
    printf("%s \n" , oFileName4);
  }

  time_t t = time(0);
  tm time = *localtime(&t);
  oFileInfo << "DATE: " << time.tm_mday << (time.tm_mon + 1) << 0 << (time.tm_year - 100) << std::endl;
  
  // * * * * * * * *
  // Check Data File for synchronocity
  // * * Get loc from data headers
  bool peekRetVal;
  int seqNum1, seqNum2;
  unsigned long long storeLocEnd;
  fileD = open(dFile, O_RDONLY);
  peekRetVal = seti_PeekDr2Header(fileD , dataHead);
  printf("  Checking for data file synchonocity...\n");
  if(peekRetVal != true) { 
    printf("ERROR peeking data file.  Exiting ...\n");
    exit(1);
  }
  seqNum1 = dataHead.dataseq;
  storeLocEnd = lseek(fileD , dr2BlockSize , SEEK_SET);
  peekRetVal = seti_PeekDr2Header(fileD , dataHead);
  seqNum2 = dataHead.dataseq;
  oFileNBS << seqNum1 << "  " << seqNum2 << std::endl;  


  // * * 
  // * * * * START OF LARGE LOOP * * * *
  // * * *
  // * * * * * * * * *
  storeLocEnd = 0;
  retVal = numSampD;
  long long loc , endLoc;
  printf(" \n Getting %lf s of data...\n  ----------------  ----------------  ----------------  \n" , double(numSamp*0.0000004) );
  for(int iter = 0; retVal == numSampD && (iter < stop || stop == 0); iter++) {
    std::vector<int> genBS(numSamp,0);
    //int chan;
    //for (telescope_id tid = AO_ALFA_0_0; tid < AO_ALFA_6_1+1; ++tid) {
    //chan = receiverid_to_channel[tid];
    for(int chan = 0; chan < numChanD; chan++) {
      //chan = 2;
      //3 & 7 seg fault; 0 & 14 & 15 work; rest retVal = 0
      if(iter == 0) {
	chanLoc[0].push_back(0); // start
	chanLoc[1].push_back(0); // end
      }
      if(chan != 8 && chan != 13) {
	fileD = open(dFile, O_RDONLY);
	
	chanLoc[0][chan] = lseek(fileD , chanLoc[1][chan] , SEEK_SET);
	endLoc = lseek(fileD , 0 , SEEK_END);
	loc = lseek(fileD , chanLoc[0][chan] , SEEK_SET);
	
	if(chan == 0) { retVal = seti_GetDr2Data(fileD, chan , numSamp, data[chan], NULL, compBlankSig, 0, true); }
	else { retVal = seti_GetDr2Data(fileD, chan , numSamp, data[chan], NULL, cw, 0, true); }
	cw.clear();
	
	chanLoc[1][chan] = lseek(fileD , 0 , SEEK_CUR);
	
	if(!retVal) {
	  printf("  Error getting channel %d \n", chan);
	  exit(1);
	} else if(retVal != numSamp) { printf("  numSamp changed to %d \n" , numSamp); }
	numSamp = retVal;
	loc = lseek(fileD, 0, SEEK_CUR);
      }
      close(fileD);
    }  // for numChan
    
    printf("  Data retrieved. Processing Blanking Signal...\n");
    
    // * * * * * 
    // * * * * * * * * Process Blanking Signal * * * * * * * * 
    // * * *  
    
    int bsHigh = 0;
    int bsLow = 0;
    for(int i = 0; i < numSamp; i++) {
      //FIX note that currently only real part is BS signal
      compBlankSig[i].real() = compBlankSig[i].real() == 1 ? 1: 0;
      blankSig.push_back((compBlankSig[i].real()));
      if(compBlankSig[i].real() == 1) {  bsHigh++; } 
      else { bsLow++; }
    }
    compBlankSig.clear();
    
    // FIX doesn't need high and low passes?

    isBS = findBlanking( blankSig, bsHigh, bsLow );
    
    // * * * * Print Blanking Signal Changes
    if(isBS == 1 && printOldBS == 1) {
      printf("  Printing Existing Blanking Signal...\n");
      for(int i = 0; i < blankSig.size(); i++) {
	if(i == 0) { oFileOBS << i << "  " << blankSig[i] << std::endl; }
	else {
	  if(blankSig[i] != blankSig[i-1]) {
	    oFileOBS << (i-1) << "  " << blankSig[i-1] << std::endl;
	    oFileOBS << i << "  " << blankSig[i] << std::endl;
	  }
	}
      } // blankSig size
    }
    
    // * * * * *
    // * * * * * * * * Process Radar Signal * * * * * * * *
    // * * *
    // * * * * * * * * *
    
    printf("  Processing Radar Signal...\n");
    
    int sigsSize = sumSignal(data , sigs , numSamp , iter);
    for(int i = 0; i < numChanD; i++) { data[i].clear(); }
    percentComp = (100*loc) / endLoc;
    printf(" at %f % \n" , percentComp);
    if(printRS == 1) {
      printf("Printing RS...\n");
      for(int i = 0; i < sigsSize; i++) {
	oFileDUR << sigs[i].dur << "  " << sigs[i].loc << std::endl;
	oFile << sigs[i].loc-1 << "  0" << std::endl;
	oFile << sigs[i].loc << "  1" << std::endl;
	oFile << sigs[i].loc+1 << "  0" << std::endl;
      }
      printf("  rs printed.  ");
    }
    printf("Processing...\n");

    unsigned long long startBSLoc, startBSLocIndex;    
    int patOccur[numPats]; // = {0 , 0 , 0 , 0} ;
    long double patFreq[numPats];
    double percentLocked;
    startBSLocIndex = 0;
    for(int jl = 0; jl < numPats; jl++) {
      patOccur[jl] = 0;
      patFreq[jl] = 0.0;
    }

    // * Find and Set Individual Pattern Types
    isRS = findRadar( sigs , patOccur);
    printf("\n Predominant Radar Type = %d , Analyzing Signal...\n" , isRS);
    
    // * Create List of Lock Points *
    printf("  Creating locked list... \n" );
    // FIX here, if percentLocked is below some threshold, must restart with lower RS thresholds etc *****
    percentLocked = findLocks( sigs , patFreq , startBSLocIndex , numSamp);
    startBSLoc = sigs[startBSLocIndex].loc;
    
    printf("  startBSLoc = %ld , startBSLocIndex = %ld \n" , startBSLoc , startBSLocIndex);
    oFileInfo << "StartBSLoc = " << startBSLoc << " Index = " << startBSLocIndex << " " << percentLocked << "% locked." << std::endl;
    printf("  %f percent of segment is locked. \n" , percentLocked);
    

    // * * * * * * * * Generate Blanking Signal * * * * * * * *
    if(percentLocked > 5.0) {
      printf(" Generating Blanking Signal...\n");
      generateBS( sigs , genBS , startBSLocIndex , startBSLoc , numSamp );
    } else {
      printf(" Insuffient locks to generate blanking signal.\n");
      oFileInfo << "Insufficient Locks to Generate BS" << std::endl;
    }
    
    // Local Clean Up  
    oFile.flush();
    if(printNewBS == 1) { oFileNBS.flush(); }
    if(printOldBS == 1) { oFileOBS.flush(); }
    if(printRS == 1) { oFileDUR.flush(); }
    sigs.clear();
    blankSig.clear();
    if( !(iter < stop-1 || stop == 0) ){
      printf("reached stop point.\n");
    } else {
      printf("        next segment...\n");
      oFileInfo << "Next Segment..." << std::endl;
    } 
    
  } // while iter
  
  // * * Closing Operations * *
  oFileInfo << "Procession Complete.";
  printf("Closing...  and oFile.good() = %d \n", oFile.good());
  oFile.flush();
  oFileInfo.flush();
  if(printNewBS == 1) { oFileNBS.flush(); }
  if(printOldBS == 1) { oFileOBS.flush(); }
  if(printRS == 1) { oFileDUR.flush(); }
  sigs.clear();
  blankSig.clear();
  oFile.close();
  oFileInfo.close();
  if(printNewBS == 1) { oFileNBS.flush(); }
  if(printOldBS == 1) { oFileOBS.flush(); }
  if(printRS == 1) { oFileDUR.flush(); }
  printf("Printing Complete. \n\n");
  
  }

// * * * * * * * * * * * * * * * * * * * * * * * * *
// * * * * *    PRIMARY FUNCTIONS    * * * * * * * *

// * * * * Transfer Data Matrix to Signal Array * * * *
// * * *
int sumSignal(std::vector<complex<int> > dataIn[] , std::vector<Signal> & sigOut , int actLength, int iterNum) {
  //Signal { long int loc; int strength; long int dur; int type; int locked; };
  int counter = 0;
  int i = 0;
  int numSummed;
  int vecLength;
  long long int totalSum = 0;
  int highSum = 0;
  long int jjj = 0;
  Signal temp = {jjj, 0 , jjj , 0 , 0};
  while(i < actLength) {
    sigOut.push_back(temp);
    sigOut.back().loc = (iterNum*numSampD + i);
    sigOut.back().strength = 0;
    numSummed = 0;
    for(int k = 0; k < binSize && i < actLength; k++,i++,numSummed++) {
      for(int j = 0; j < numChanD; j++) {
	if(j != 8 && j != 13) {
	  dataIn[j][i].real() = dataIn[j][i].real() == 1 ? 1: 0;
	  dataIn[j][i].imag() = dataIn[j][i].imag() == 1 ? 1: 0;
	  sigOut.back().strength += dataIn[j][i].real();
	  sigOut.back().strength += dataIn[j][i].imag();
	}
      }
    }
    totalSum += sigOut.back().strength;
    sigOut.back().strength /= numSummed;
    if( sigOut.back().strength > highSum) { highSum = sigOut.back().strength; }
    
    long tempDur, tempLoc;
    tempLoc = sigOut.size() - 1;
    if(tempLoc > 0 && counter >= 3) { tempDur = sigOut[tempLoc].loc - sigOut[tempLoc-1].loc; }
    else { tempDur = 0; }
    if(sigOut.back().strength > thresh && (tempDur >= igLow || counter < 3) ){
      i++;
      if(counter < 4) { counter++; }
      sigOut.back().dur = tempDur;
    } else {
      sigOut.pop_back();
    }
  } // while actLength

  vecLength = sigOut.size();
  long double average = (((long double)totalSum)/((long double)actLength));
  printf("   signal summed. \n");
  printf(" HighSum = %d , average = %Lf , number of = %d \n" , highSum , average, vecLength);
  oFileInfo << "HighSum = " << highSum << " , average = " << average << " , number of = " << vecLength << std::endl;

  return(vecLength);
}

// * * * * * Find Blanking Signal * * * * *
// * * *
int findBlanking( std::vector<int> inBlank, int iBSHigh, int iBSLow ) {
  //FIX add find type of blanking signal maybe??
  int bsDiff;
  if(iBSHigh > iBSLow) { bsDiff = iBSHigh - iBSLow; }
  else if(iBSHigh < iBSLow) { bsDiff = iBSLow - iBSHigh; }
  else { bsDiff = 0; }

  int same,numLong,bsLengthThresh;
  long length,longest;
  bsLengthThresh = 200;
  same = 0;
  length = 0;
  longest = 0;
  numLong = 0;
  printf(" BS High %d , Low %d \n" , iBSHigh , iBSLow);
  oFileInfo << "BS high = " << iBSHigh << " , low = " << iBSLow << std::endl;
  if(iBSLow < 100) {
    isBS = 0;
    printf(" BS BAD: constant high. \n");
  } else if(iBSHigh < 100) {
    isBS = 0;
    printf(" BS BAD: constant low. \n");
  } else if(bsDiff < 100) {
    isBS = 0;
    printf(" BS BAD: random low = %d , high = %d. \n" , iBSLow, iBSHigh);
  } else {
    for(int i = 1; i < inBlank.size(); i++) {
      if(same == 0 && inBlank[i] == inBlank[i-1]) {
	same = 1;
	length = 1;
      } else if(inBlank[i] == inBlank[i-1]) { length++; }
      else if(same == 1 && inBlank[i] != inBlank[i-1]) {
	if(length > bsLengthThresh) { numLong++; }
	if(length > longest) { longest = length; }
	same = 0;
      }
    } // for inBlank.Size                                                                                                                         
    if(numLong > 5) {
      isBS = 1;
      printf(" BS GOOD: ");
    } else {
      isBS = 0;
      printf(" BS BAD: ");
    }
    if(isBS == 1) { oFileInfo << "BS Good."; }
    else { oFileInfo << "BS Bad."; }
    printf("\n   number above %d = %d, with longest = %ld \n" , bsLengthThresh , numLong , longest);
    oFileInfo << " number above " << bsLengthThresh << " = " << numLong << " , with longest = " << longest << std::endl;
  } // hi low if else if
  
  return isBS;

}



// * * * * * Find Radar Signal and Set Individual Pat Type * * * * *
// * * *
int findRadar( std::vector<Signal> & sigsIn , int *patsOut) {
  int tIsRS = 0;
  int nGoodPats = 0;
  int tempType, bigType, smallType;
  int mostOcc = 0;
  int lastDur[3] = {20 , 0 , 0};
  unsigned long long int aveDur[numPats][numAlts][2];
  for(int i = 0; i < numPats; i++) { for(int j = 0; j < numAlts; j++) { for(int k = 0; k < 2; k++) {
	aveDur[i][j][k] = 0; 
      } } }
  
  int sigsSize = sigsIn.size();
  if( !(sigsSize < 20) ) {
    for(int i = 1; i < sigsSize; i++) {
      std::vector<int> matches;
      int control1 = 0;
      int control2 = 0;
      for(int j = 1; j < numPats; j++) {
	for(int k = 0; k < numAlts; k++) {
	  // Make list of possible matches
	  if( expDur[j][k] != 0 && compDur(sigsIn[i].dur , expDur[j][k] , slack) ) { matches.push_back( j*10 + k); }
	} //numPats (j)
      } //numAlts (k)
      
      //If no matches, no signal
      if(matches.size() == 0) { sigsIn[i].type = 0; }
      else {
	tempType = matches[0];
	//If one match, thats the pattern
	if(matches.size() == 1) { sigsIn[i].type = tempType; }
	//If multiple matches
	else {
	  //check for precedent
	  for(int m = 0; m < matches.size(); m++) {
	    tempType = matches[m];
	    tempType = getLastType(tempType);
	    bigType = getBigType(tempType);
	    smallType = getSmallType(tempType);
	    //printf("matches[m] = %d , tempType = %d , big = %d , small = %d \n" , matches[m] , tempType , bigType , smallType);
	    //printf(" i-1 dur = %ld , expDur = %ld \n" , sigsIn[i-1].dur , expDur[bigType][smallType]);
	    if( compDur(sigsIn[i-1].dur , expDur[bigType][smallType] , slack) ) {
	      control1++;
	      sigsIn[i].type = getNextType(tempType);
	    } 
	  } //m < matches.size
	  //if none or multiple precedent matches, look to the future
	  if(control1 != 1) {
	    for(int m = 0; m < matches.size(); m++) {
	      tempType = matches[m];
	      tempType = getNextType(tempType);
	      bigType = getBigType(tempType);
	      smallType = getSmallType(tempType);
	      //printf(" i+1 dur = %ld , expDur = %ld \n" , sigsIn[i+1].dur , expDur[bigType][smallType]);
	      if( compDur(sigsIn[i+1].dur , expDur[bigType][smallType] , slack) ) {
		control2++;
		sigsIn[i].type = getLastType(tempType);
	      }
	    } //m < matches.size
	    //if still inadequite matches, pick first match
	    //FIX if both before and after match numerous - could check between those 2 to see if correspondence
	    if(control2 != 1) {
	      printf("Error on precedent match.  %d matches backwards, %d forwards.  Guessing type %d . \n" , control1 , control2, matches[0]);
	      oFileInfo << "Error on precedent match. " << control1 << " matches backwards, " << control2 << " forwards.  Guessing type " << matches[0] << std::endl;
	      printf(" dur before after = %d , %d \n" , sigsIn[i-1].dur , sigsIn[i+1].dur);
	      sigsIn[i].type = matches[0];
	    }
	  } //control1 != 1
	} // matches.size == 1
      } // matches.size == 0

      int cont1;
      //Look for simple, unrecogonized patterns
      if(sigsIn[i].type == 0 && i > 3 && lastDur[1] > lastDur[0]) {
	if( compDur(sigsIn[i].dur , lastDur[1] , lastDur[0]) && compDur(sigsIn[i].dur , lastDur[2] , lastDur[0]) ) {
	  //Make sure not already found
	  cont1 = 0;
	  if(newPat.size() > 0) { for(int gh = 0; gh < newPat.size(); gh++) { if( compDur(newPat[gh] , lastDur[1] , lastDur[0]) ) { cont1 = 1; } } }
	  //if new
	  if(cont1 == 0) { 
	    newPat.push_back( ((lastDur[1] + lastDur[2] + sigsIn[i].dur)/3) );
	    printf("**** Possible New Pattern @ ind %d, loc %ld.  Durations = %d , %d , %d  --  %d ****\n" ,
		   i-2, sigsIn[i-2].loc , lastDur[2] , lastDur[1] , sigsIn[i].dur , newPat.back() ); 
	  }	
	} // if last 3 match
      }   //type == 0

      tempType = sigsIn[i].type;
      //Find averages of found signals
      if(tempType != 0) {
	bigType = getBigType(tempType);
	smallType = getSmallType(tempType);
	aveDur[bigType][smallType][0] += sigsIn[i].dur;
	aveDur[bigType][smallType][1]++;
      }
      lastDur[2] = lastDur[1];
      lastDur[1] = sigsIn[i].dur;
    } // i < sigsSize 

    //Adapt expected durations to observed averages
    for(int i = 0; i < numPats; i++) {
      for(int j = 0; j < numAlts; j++) {
	if(aveDur[i][j][1] > mostOcc) {
	  mostOcc = (int)(aveDur[i][j][1]);
	  tIsRS = i;
	}
	if(aveDur[i][j][1] != 0) { 
	  oFileInfo << "Pat " << i << " , " << j << " occured " << aveDur[i][j][1] << " times." << std::endl;
	  //printf("  Pat %d , %d  occured %ld times.\n" , i , j , aveDur[i][j][1]);
	}
	if(aveDur[i][j][1] > 100) {
	  aveDur[i][j][0] = (long long int)(aveDur[i][j][0]/aveDur[i][j][1]);
	  actDur[i][j] = (int)(aveDur[i][j][0]);
	  if(expDur[i][j] != actDur[i][j]) {
	    printf("  Pat %d , %d  occured %ld times. Recalibrated from %ld to %d. \n" , i , j , aveDur[i][j][1] , expDur[i][j] , actDur[i][j]);
	    oFileInfo << "Pat " << i << " , " << j << " Recalibrated from " << expDur[i][j] << " to " << actDur[i][j] << std::endl;
	  }
	} else {
	  actDur[i][j] = expDur[i][j];
	}
      } // j < numAlts
    }   // i < numPats
  }     // sigsSize > 20
  return(tIsRS); //return dominant pattern
}


// * * * * * Create List of Lock Points * * * * * *
// * * * 
double findLocks( std::vector<Signal> & sigsIn , long double *patsFreq , unsigned long long int &oIndex , int numSamp) {
  long double pLocked = 0.0;
  long int pLength = 0;
  int tempType, bigType;
  int sigsSize = sigsIn.size();
  long int nLockPoints = 0;
  long int startLocked = 0;
  for(int i = 4; i < sigsSize; i++) {
    //If previous not locked
    if(sigsIn[i-1].locked == 0) {
      tempType = sigsIn[i-3].type;
      startLocked = sigsIn[i-3].loc;
      bigType = getBigType( tempType );
      for(int j = 2; j > -1; j--) {
	tempType = getNextType( tempType );
	//if matches expected, temporarily locked.
	if( tempType != 0 && sigsIn[i-j].type == tempType ) { 
	  sigsIn[i-j].locked = bigType;
	  nLockPoints++;
	  //Set first lock point
	  if(j == 0 && oIndex == 0) { oIndex = (i-3); }
	} 
	//if does not match, un lock temporarily locked, move on
	else { 
	  while(j < 2) { 
	    j++;
	    sigsIn[i-j].locked = 0;
	    nLockPoints--;
	  }
	  j = -1;
	  startLocked = 0;
	}
      } // for j > -1
      if(startLocked != 0) {
	//printf(" Initial Lock onto signal @ loc = %ld , index = %d , type = %d \n" , startLocked , i-3 , sigsIn[i-3].type);
	oFileInfo << "Initial Lock onto signal @ loc = " << startLocked << " , index = " << i-3 << " , type = " << sigsIn[i-3].type << std::endl;
      }
    } // .locked == 0
    // if previous is locked
    else {
      tempType = sigsIn[i-1].type;
      bigType = getBigType(tempType);
      tempType = getNextType(tempType);
      //matches expected type
      if(tempType != 0 && sigsIn[i].type == tempType) {
	sigsIn[i].locked = bigType;
	nLockPoints++;
      } 
      //does not match expected type
      else {
	sigsIn[i].locked = 0;
	pLength = (sigsIn[i-1].loc - startLocked);
	patsFreq[bigType] += pLength;          //track # samples locked on each pattern
	pLocked += pLength;                    //  "   #   "       "     total (all patterns)
	//printf("  Lock lost @ (last good) loc = %ld , index = %d , lockedLength = %Lf \n" , sigsIn[i-1].loc , i-1 , pLength);
	oFileInfo << "Lock lost @ (last good) loc = " << sigsIn[i-1].loc << " , index = " << i-1 << " , lockedLength = " << pLength << std::endl;
      }
    }
    if(i == sigsSize && sigsIn[i].locked != 0) { 
      pLocked += (sigsIn[i].loc - startLocked);
      patsFreq[bigType] += pLength;
    }
  } // for i < sumSig size

  printf("   lock list complete, with %ld locks.\n" , nLockPoints);
  oFileInfo << "Number locks = " << nLockPoints << std::endl;
  //Average
  pLocked /= (long double)(numSamp/100);
  for(int i = 0; i < numPats; i++) { 
    patsFreq[i] /= (long double)(numSamp/100);
    if(patsFreq[i] > 0.0 ) {
      printf("    Pattern # %d locked for %Lf percent.\n" , i , patsFreq[i]);
      oFileInfo << "Pat " << i << " locked for " << patsFreq[i] << " percent." << std::endl;
    }
  }
  return pLocked;
}


// * * * * * Generate Blanking Signal * * * * *
// * * *
void generateBS( std::vector<Signal> sigsIn , std::vector<int> & genBlank , 
		 unsigned long int startIndex , unsigned long long int startLoc , int numSamp) {
  
  int patIndUp, patIndDown;      //current RS pattern in both directions
  long int locUp = startLoc;         //current upwards blanking location
  long int locDown = startLoc;       //current downward blanking location
  long int tempInd = startIndex;
  double relStart, relEnd, blength;  //relative blanking length before and after start location
  int actStart, actEnd;              //absolute position of blanking 
  int dex = 0;
  int nRelocks = 0;              //number of times relocked
  int lockAction = 1;            //current aspect of process - 0:search for lock || 1:continue lock || 2:continue up-down blanking
  int lastLockAction = 1;
  long lastLockInd, thisLockInd, nextLockInd; //indices of current and next lock points
  long lengthNoLock, switchLoc;  //length between current and next lock points and half of that (switch point between up and down blanking)
  long nextDur = 0;              //length of next period
  long next2Dur = 0;             //length of next 2 periods
  long stopLocDown = 0;          //downward blanking stop location (default to abs begin)
  long stopLocUp = numSamp;      //upward blanknig stop loc (default to abs end)
  blength = (double)blankLength;
  relStart = relStartLoc*blength;
  relEnd = (1-relStartLoc)*blength;

  // * Starting values
  patIndUp = sigsIn[startIndex].type;
  patIndDown = sigsIn[startIndex].type;
  locUp = startLoc;
  locDown = startLoc;
  tempInd = startIndex;
  //find next lock point
  while( (sigsIn[dex].locked == 0 || sigsIn[dex-1].locked == 0 || sigsIn[dex].loc <= locUp) && dex < sigsIn.size() ) { dex++; }
  nextLockInd = dex;
  stopLocUp = sigsIn[nextLockInd].loc;

  printf(" after 45 = %d, after 46 = %d, after 40 = %d, after 41 = %d, after 42 = %d \n" , 
	 getNextType(45) , getNextType(46) ,getNextType(40) ,getNextType(41) ,getNextType(42) );
  
  // * * Whole blanking loop
  while( locDown > 0 || locUp < numSamp) {
    //printf("\n locUp = %ld , locDown = %ld || stopUp = %ld , stopDown = %ld || patIndUp = %d , patIndDown = %d \n " ,
    //locUp , locDown , stopLocUp , stopLocDown, patIndUp , patIndDown );

    // * Blank Downward
    if(locDown > 0 && locDown > stopLocDown) {
      //printf("Blanking down @ locDown = %ld \n" , locDown);
      actStart = (int)(locDown - relStart);
      actEnd = (int)(locDown + relEnd);
      if(actStart < 0) { actStart = 0; }
      if(actEnd > numSamp) { actEnd = numSamp; }
      for(int i = actStart; i < actEnd; i++) { genBlank[i] = 1; }
      if(printNewBS == 1) {
	oFileNBS << actStart-1 << "  " << 0 << std::endl;
	oFileNBS << actStart << "  " << 1 << std::endl;
	oFileNBS << actEnd << "  " << 1 << std::endl;
	oFileNBS << actEnd+1 << "  " << 0 << std::endl;
      } 
    }

    // * If between lock points, see if still
    if(lockAction == 1 && locUp >= (stopLocUp - (getNextDur(patIndUp)/2) ) && locDown <= stopLocDown) {
      //printf("lockAction 1 --> 0  locUp changed to %ld\n" , sigsIn[nextLockInd].loc); 
      oFileInfo << "LockAction 1 --> 0  locUp changed from " << locUp << " , to " << sigsIn[nextLockInd].loc << std::endl;
      lockAction = 0;
      nRelocks++;
      locUp = sigsIn[nextLockInd].loc;
      stopLocUp = numSamp;
      patIndUp = sigsIn[nextLockInd].type;
      //Dont blank downwards while stepping upward between lock points
      locDown = 0;
      stopLocDown = 0;
    }
    
    // * If not between lock points
    if(lockAction == 0) {
      
      nRelocks++;
      lastLockInd = thisLockInd;
      thisLockInd = nextLockInd;

      locUp = sigsIn[thisLockInd].loc;
      patIndUp = sigsIn[thisLockInd].type;

      //printf(" lockAction == 0 , nextLockInd = %d   loc = %ld " , nextLockInd, sigsIn[nextLockInd].loc);
      while( (sigsIn[dex].locked == 0 || sigsIn[dex].loc <= locUp) && dex < sigsIn.size() ) { dex++; }
      nextLockInd = dex;
      //printf(" , nextLockInd = %d , loc = %ld , " , nextLockInd, sigsIn[nextLockInd].loc);

      //stopLocUp = sigsIn[nextLockInd].loc;
      //printf("stopLocUp = %ld \n" , stopLocUp);
      locDown = 0;
      stopLocDown = 0;
      
      nextDur = getNextDur(patIndUp);
      next2Dur = getNext2Dur(patIndUp);
      lengthNoLock = sigsIn[nextLockInd].loc - sigsIn[thisLockInd].loc;
      
      //if enough space between locks, start blanking up and down to half-way point
      if(lengthNoLock >= next2Dur) {
	//printf("lockAction 0 --> 1 , lengthNoLock = %ld , locUp = %ld , locDown = %ld , stopLocUp = %ld , stopLocDown = %ld \n" ,
	//lengthNoLock, locUp , locDown, stopLocUp , stopLocDown );
	oFileInfo << "LockAction 0 --> 1  blanking between " << sigsIn[thisLockInd].loc << " , and " << sigsIn[nextLockInd].loc << " length = " << lengthNoLock << std::endl;
	lockAction = 1;
	switchLoc = (long int)((sigsIn[nextLockInd].loc + sigsIn[thisLockInd].loc)/2);    //calculate half-way points
	stopLocDown = switchLoc;
	stopLocUp = switchLoc;
	locUp = sigsIn[thisLockInd].loc;     //shouldn't be doing anything
	patIndUp = sigsIn[thisLockInd].type; //same
	locDown = sigsIn[nextLockInd].loc;
	patIndDown = sigsIn[nextLockInd].type;
      } //else { lockAction = 0; } //if not enough space, continue to next lock
      
      //if reached end of lock list, go to end uninterupted
      // FIX might need patIndType determination
      if(dex >= sigsIn.size() ) {
	//printf("Setting end.\n");
	lockAction = 1;
	stopLocUp = numSamp;
      }
      
    } // lockAction == 0

    //report lock action changes
    if(lockAction != lastLockAction) { 
      //printf("Lock Action %d ---> %d   @ %ld \n" , lastLockAction , lockAction , locUp);
      //oFileInfo << "Lock Action " << lastLockAction << " ---> " << lockAction << " @ " << locUp << std::endl;
    }
    lastLockAction = lockAction;
      
    // * Blank Upward
    if(locUp < numSamp && locUp != locDown && locUp < stopLocUp) {
      //printf(" Blanking up @ locUp = %ld , pat = %d \n" , locUp, patIndUp);
      actStart = (int)(locUp - relStart);
      actEnd = (int)(locUp + relEnd);
      if(actStart < 0) { actStart = 0; }
      if(actEnd > numSamp) { actEnd = numSamp; }
      
      for(int i = actStart; i < actEnd; i++) { genBlank[i] = 1; }
      if(printNewBS == 1) {
	oFileNBS << actStart-1 << "  " << 0 << std::endl;
	oFileNBS << actStart << "  " << 1 << std::endl;
	oFileNBS << actEnd << "  " << 1 << std::endl;
	oFileNBS << actEnd+1 << "  " << 0 << std::endl;
      }
    }
    
    // * * * Cycle Through Pattern(s) * * *
    int bigType, smallType;
    if(locUp < numSamp && locUp < stopLocUp) {
      patIndUp = getNextType(patIndUp);
      bigType = getBigType(patIndUp);
      smallType = getSmallType(patIndUp);
      locUp += actDur[bigType][smallType];
    }
    if(locDown > 0 && locDown > stopLocDown) {
      bigType = getBigType(patIndDown);
      smallType = getSmallType(patIndDown);
      locDown -= actDur[bigType][smallType];
      patIndDown = getLastType(patIndDown);
    }

  } //while loc Up and Down
  oFileInfo << "Blanking Signal Generated. " << nRelocks << " relocks." << std::endl;
  printf(" Blanking Signal generated.  %d relocks. \n" , nRelocks);
}

// * * * * * * * * * * * * * * * * * * * * * * * * *
// * * * * *    SECONDARY FUNCTIONS    * * * * * * *

// * * * * * Compare Durations * * * * * *
bool compDur(int dur1 , int dur2, int slck) {
  bool match = false;
  if(dur1 > (dur2 - slck) && dur1 < (dur2 + slck) ) {
    match = true;
  }
  return match;
}
 
// * * * * * Get Large Pattern Type * * * * * *
int getBigType(int smallType) {
  int bigType = 0;
  bigType = (int)(smallType/10);
  return bigType;
}

// * * * Get Small Pattern Type * * *
int getSmallType(int totalType) {
  int smallType,bigType;
  bigType = getBigType(totalType);
  smallType = totalType - 10*bigType;
  return smallType;
}

// * * * Get Expected Next Pattern * * *
int getNextType(int nowType) {
  int nextType = 0;
  int bigType, smallType;
  int limit = 0;
  bigType = getBigType(nowType);
  smallType = getSmallType(nowType);
  while( expDur[bigType][limit] != 0 && limit < numAlts) { limit++; }
  if(smallType < (--limit)) { nextType = smallType+1; }
  else { nextType = 0; }
  nextType += bigType*10;
  return nextType;
}

// * * * Get Expected Previous Pattern * * *
int getLastType(int nowType) {
  int firstOne;
  int lastType = 0;
  int limit = 0;
  int bigType, smallType;
  bigType = getBigType(nowType);
  smallType = getSmallType(nowType);
  firstOne = expDur[bigType][0];
  while( expDur[bigType][limit] != 0 && limit < (numAlts) ) { limit++; }
  if(smallType > 0) { lastType = smallType-1; }                                                                                                                              
  else { lastType = (--limit); }
  lastType += 10*bigType;
  return lastType;
}

// * * * Get Expected Next Duration * * *
long int getNextDur(int nowType) {
  long int dur;
  int bigType, smallType;
  nowType = getNextType(nowType);
  bigType = getBigType(nowType);
  smallType = getSmallType(nowType);
  dur = actDur[bigType][smallType];
  return dur;
}

// * * * Get Expected Next 2 Durations * * *
 long int getNext2Dur(int nowType) {
   long int dur2;
   int bigType, smallType;
   nowType = getNextType(nowType);
   bigType = getBigType(nowType);
   smallType = getSmallType(nowType);
   dur2 = actDur[bigType][smallType];
   nowType = getNextType(nowType);
   bigType = getBigType(nowType);
   smallType = nowType - 10*bigType;
   dur2 += actDur[bigType][smallType];
   return dur2;
 } 
