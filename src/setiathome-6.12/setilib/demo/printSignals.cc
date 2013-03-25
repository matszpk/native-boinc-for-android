// * * * * * * * * * * * * * * * * * * *
//
// Luke Zoltan Kelley
// Created: 062508
// Updated: 062508
//
// * * * * * * * * * * *

#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "setilib.h"
#include<vector>
#include<complex>
#include<string>
#include <iostream>
#include <fstream>

int fileD, channel, sample;
int retVal,retVal2,retVal3,retVal4;
char * dFile;
//const int numSampD = 650000001;  //each samp = 12 bytes; 1MB = 1,048,576 bytes; 80000 samples = 960,000 bytes = .91 MB
//const int numSampD = 62500000;
//const int numSampD = 50000000;
const int numSampD = 5000000;
const int numChanD = 16;
int binSize = 10;
int thresh = 22;
int stop = 5;                      //0 = complete file; # = number of iterations (iter) to procede through


const int lowDur1 = 5580;
const int lowDur2 = 6050;
const int lowDur3 = 5860;
const int lowDur4 = 5482;
const int lowDur5 = 7270;
const int lowDur6 = 7000;
const int slack = 100;

int numSamp, numChan, highSum, isBS, isRS;
long double average, percentComp;
long storeLocEnd, storeLocStart, endLoc, bsHigh, bsLow;
//char * oFileName = (char *) malloc(strlen("first string data blah blah blah blah blah\n") + 1);
int genBlankSig(std::vector<complex<int> >, std::vector<int>);
int recordBlankSig(std::vector<int>,char *);

std::vector<complex<int> > cw;


//std::ofstream oFile ("statstat.dat");


int main(int argc, char ** argv) {
  printf("\nPrint Signal Started. \n\n");

  //char * oFileName = (char *) malloc(strlen("first string data blah blah blah blah blah\n") + 1);
  char oFileName[256];

  if(oFileName == NULL) {
    printf("oFileName = null");
    exit(1);
  }

  /*
  std::ofstream oFile ("statstat.dat");
  if(oFile.is_open() && oFile.good()) {
    printf("  Output file created:  ");
  } else {
    printf("  Error creating output file...Exiting\n");
    oFile.close();
    exit(1);
  }
  printf("%s \n" , oFileName);
  */
  
  //Parse Command Line
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-f")) {
      dFile = argv[++i];
    } else if (!strcmp(argv[i], "-nS")) {
      numSamp = atoi(argv[++i]);
    } else if (!strcmp(argv[i], "-nC")) {
      numChan = atoi(argv[++i]);
    } else {
      fprintf(stderr, "usage : blankSignal -f filename -nS number_samples -nC number_channels\n");
      exit(1);
    }
  }
  if (dFile == NULL) { 
    fprintf(stderr, "usage : blankSignal -f filename -nS number_samples -nC number_channels\n");
    exit(1);
  }
  //Update Parameters
  if(numSamp == NULL) {
    numSamp = numSampD;
  }
  if(numChan == NULL) {
    numChan = numChanD;
  }

  std::vector<complex<int> > data[numChan];  //array of complex vectors, array size = numSamp; ea vector size = numChan
  complex<int> temp;
  std::vector<complex<int> > compBlankSig;
  std::vector<int> blankSig;
  std::vector<int> genBS(numSamp,0);
  std::vector<unsigned long long> sumSig[4];
  std::vector<int> largestSums(20,0);
  std::vector<long> largestSumsLoc(10,0);
  std::vector<complex<int> > signal;
  int largestLength = largestSums.size();

  // * * * * * * * * Getting Data * * * * * * * *
  // * * * * * *


  //Initialize Data File
  fileD = open(dFile, O_RDONLY);
  if(fileD == -1) {
    printf("  Error opening data file ");
    exit(1);
  } else {
    printf("  Data file opened:  ");
  }
  printf("%s \n", dFile);

  
  strcpy(oFileName, "Status_");
  strcat(oFileName,dFile);
  if(remove("Status_backup.txt")) {}
  if(rename(oFileName, "Status_backup.txt")) {}
  std::ofstream oFile (oFileName);
  //std::ofstream oFile ("statstat.dat");
  if(oFile.is_open() && oFile.good()) {
    printf("  Output file created:  ");
  } else {
    printf("  Error creating output file...Exiting\n");
    oFile.close();
    exit(1);
  }
  printf("%s \n" , oFileName);


  // * * * 
  // * * * * START OF LARGE LOOP * * * *
  // * * *

  storeLocEnd = 0;
  retVal = numSampD;
  for(int iter = 0; retVal == numSampD && (iter < stop || stop == 0); iter++) {
    //printf(" \n  Getting %lf s of data...\n\n" , double(numSamp*0.0000004) );
    long loc;
    //Get Matrix of data
    for(int i = 0; i < numChan; i++) {
      //printf("    channel %d " , i);
      if(i != 8 && i != 13) {
	fileD = open(dFile, O_RDONLY);
	if(i == 0) {
	  storeLocStart = lseek(fileD , storeLocEnd , SEEK_SET);
	}
	endLoc = lseek(fileD , 0 , SEEK_END);
	loc = lseek(fileD, storeLocStart, SEEK_SET);
	//printf("  Location before: %ld  " , loc);
	retVal = seti_GetDr2Data(fileD, i , numSamp, data[i], NULL, cw, 0, false);
	cw.clear();
	if(i == 0) {
	  storeLocEnd = lseek(fileD , 0 , SEEK_CUR);
	  printf("\nstart @ %ld , end @ %ld \n" , storeLocStart , storeLocEnd);
	}

	if(!retVal) {
	  printf("  Error getting channel %d \n", i);
	  exit(1);
	} else if(retVal != numSamp) {
	  numSamp = retVal;
	  printf("  numSamp changed to %d \n" , numSamp);
	}      
	//printf("   retrieved...  ");
	loc = lseek(fileD, 0, SEEK_CUR);
	//printf("  Location after: %ld \n" , loc);
      } else {
	//printf("\n");
      }
      close(fileD);
    }
    //printf("  Getting blanking signal.");
    
    //Get Blanking Data
    if( (fileD = open(dFile, O_RDONLY)) == -1) {
      printf("  Error opening data file ");
      exit(1);
    }
    retVal = seti_GetDr2Data(fileD, 13 , numSamp, compBlankSig, NULL, cw, 0, false);
    cw.clear();
    if(!retVal) {
      printf("  Error getting blanking signal\n");
      exit(1);
    }
    //close(fileD);
    
    //printf("  Blanking Data Retreived... processing...\n");

    // * * * * * 
    // * * * * * * * * Processing Blanking Signal * * * * * * * * 
    // * * *  
    
    bsHigh = 0;
    bsLow = 0;
    for(int i = 0; i < numSamp; i++) {
      compBlankSig[i].imag() = compBlankSig[i].imag() == 1 ? 1: 0;
      blankSig.push_back((compBlankSig[i].imag()));
      if(compBlankSig[i].imag() == 1) {
	bsHigh++;
      } else {
	bsLow++;
      }
    }
    compBlankSig.clear();
    int bsDiff;
    if(bsHigh > bsLow) {
      bsDiff = bsHigh - bsLow;
    } else if(bsHigh < bsLow) {
      bsDiff = bsLow - bsHigh;
    } else {
      bsDiff = 0;
    }


    int same,numLong,bsLengthThresh;
    long length,longest;
    bsLengthThresh = 200;
    same = 0;
    length = 0;
    numLong = 0;
    if(bsLow < 10) {
      isBS = 0;
      printf("  BS BAD: constant high. \n");
      oFile << "  BS BAD: constant high." << std::endl;
    } else if(bsHigh < 10) {
      isBS = 0;
      printf("  BS BAD: constant low. \n");
      oFile << "  BS BAD: constant low." << std::endl;
    } else if(bsDiff < 100) {
      isBS = 0;
      printf("  BS BAD: random low = %d , high = %d. \n" , bsLow, bsHigh);
      oFile << "  BS BAD: random low = " << bsLow << " , high = " << bsHigh << "." << std::endl;
    } else {
      for(int i = 1; i < blankSig.size(); i++) {
	if(same == 0 && blankSig[i] == blankSig[i-1]) {
	  same = 1;
	  length = 1;
	} else if(blankSig[i] == blankSig[i-1]) {
	  length++;
	} else if(same == 1 && blankSig[i] != blankSig[i-1]) {
	  if(length > bsLengthThresh) {
	    numLong++;
	  }
	  if(length > longest) {
	    longest = length;
	  }
	  same = 0;
	}
      } // for blankSig.Size
      if(numLong > 5) {
	isBS = 1;
	printf("  BS GOOD: ");
	oFile << "  BS GOOD: " << std::endl;
      } else {
	isBS = 0;
	printf("  BS BAD: ");
	oFile << "  BS BAD: " << std::endl;
      }
      printf(" number above %d = %d, with longest = %ld \n" , bsLengthThresh , numLong , longest);
      oFile << " number above " << bsLengthThresh << " = " << numLong << ", with longest = " << longest << "." << std::endl;
    } //if else if else
    

    // * * * * *
    // * * * * * * * * Summing Signal and Printing Results * * * * * * * *
    // * * *
    
    int i = 0;
    int numSummed;
    highSum = 0;
    average = 0;
    while(i < numSamp) {
      sumSig[0].push_back(iter*numSampD + i);  //location
      sumSig[1].push_back(0);
      sumSig[2].push_back(0);
      sumSig[3].push_back(0);
      numSummed = 0;
      for(int k = 0; k < binSize && i < numSamp; k++,i++,numSummed++) {
	for(int j = 0; j < numChan; j++) {
	  if(j != 8 && j != 13) {
	    data[j][i].real() = data[j][i].real() == 1 ? 1: 0;
	    data[j][i].imag() = data[j][i].imag() == 1 ? 1: 0;
	    sumSig[1].back() += data[j][i].real();
	    sumSig[1].back() += data[j][i].imag();
	  }
	}
      }
      sumSig[1].back() /= numSummed;
      average += sumSig[1].back();
      if( sumSig[1].back() > highSum) {
	highSum = sumSig[1].back();
      }
      if(sumSig[1].back() > thresh) {
	i++;
	//Make list of 10 longest & corresponding locations
	int listStop = 0;
	for(int l = 0; l < largestLength && listStop == 0; l++) {
	  if(sumSig[1].back() > largestSums[l]) {
	    largestSums[l] = sumSig[1].back();
	    largestSumsLoc[l] = sumSig[0].back();
	    listStop = 1; // no duplicates
	  }
	}
      } else {
	sumSig[0].pop_back();
	sumSig[1].pop_back();
	sumSig[2].pop_back();
	sumSig[3].pop_back();
      }
    } // while numSamp
    for(int i = 0; i < numChan; i++) {
      data[i].clear();
    }

    //printf("0 size = %d , 1 size = %d, 2 size = %d, 3 size = %d" , sumSig[0].size()  , sumSig[1].size() , sumSig[2].size() , sumSig[3].size());


    percentComp = (100*loc) / endLoc;
    average /= numSamp;
    //printf("  sumSig created. printing...  \n");
    for(int i = 0; i < sumSig[0].size(); i++) {
      if(i > 0) {
	sumSig[2][i] = (sumSig[0][i] - sumSig[0][i-1]);
      } else {
	sumSig[2][i] = 0;
      }
      if(sumSig[2][i] > 20) {    //should be take out eventually	
	//printf(" i = %d , sumSig[2][i] = %ld , sumSig[0][i] = %ld , sumSig[1][i] = %ld \n" , i , sumSig[2][i], sumSig[0][i] , sumSig[1][i]);
	oFile << sumSig[2][i] << "  " << sumSig[0][i] << "  " << sumSig[1][i] << std::endl;
	//if(oFile.good() == 0) {printf("  -- Output good = %d \n" , oFile.good() ); }
      }
    }
    if(oFile.good() == 0) {printf("  -- Output good = %d \n" , oFile.good() ); }
    oFile << "  - - - " << highSum << "  " << average << "  " << percentComp << " - - - " << std::endl;
    printf("    HighestSum = %d , Average = %Lf , percent = %Lf\n" , highSum , average , percentComp);
    //printf("  Printing Segment complete. \n");

    // * * * * *
    // * * * * * * * * * * Look for Radar Signal * * * * * * * *
    // * * * 

    //printf("  Examining sumSig for Radar... \n");
    int nGoodPats = 0;
    int foundLong = 0;
    int size = sumSig[0].size();
    long peakDiff;
    if(size < 2) {
      isRS = 0;
      printf("  RS BAD: not enough above threshold.\n");
      oFile << "  RS BAD: not enough above threshold." << std::endl;
    } else {
      for(int i = 0; i < size; i++) {
	//Match low durations w/ patterns
	if( (sumSig[2][i] > lowDur1 - slack) && (sumSig[2][i] < lowDur1 + slack) ) {
	  sumSig[3][i] = 1;
	  nGoodPats++;
	} else if( (sumSig[2][i] > lowDur2 - slack) && (sumSig[2][i] < lowDur2 + slack) ) {
          sumSig[3][i] = 2;
	  nGoodPats++;
	} else if( (sumSig[2][i] > lowDur3 - slack) && (sumSig[2][i] < lowDur3 + slack) ) {
          sumSig[3][i] = 3;
	  nGoodPats++;
	} else if( (sumSig[2][i] > lowDur4 - slack) && (sumSig[2][i] < lowDur4 + slack) ) {
          sumSig[3][i] = 4;
	  nGoodPats++;
	} else if( (sumSig[2][i] > lowDur5 - slack) && (sumSig[2][i] < lowDur5 + slack) ) {
          sumSig[3][i] = 5;
	  nGoodPats++;
	} else if( (sumSig[2][i] > lowDur6 - slack) && (sumSig[2][i] < lowDur6 + slack) ) {
          sumSig[3][i] = 6;
	  nGoodPats++;
	}	    
      } // for size
      

      for(int i = 0; i < largestLength; i++) {
	for(int j = 0; j < largestLength; j++) {
	  if(largestSumsLoc[i] > largestSumsLoc[j]) {
	    peakDiff = largestSumsLoc[i] - largestSumsLoc[j];
	  } else {
	    peakDiff = largestSumsLoc[j] - largestSumsLoc[i];
	  }
	  if( peakDiff > 28750000 && peakDiff < 31250000 ) {
	    foundLong = 1;
	  }
	} // for 10
      } // for 10
      //} //if size > 2


      if(nGoodPats > 3 && foundLong == 1) {
	isRS = 1;
	printf("  RS GOOD: %d patterns recognized, 12s period found.\n" , nGoodPats);
	oFile << "  RS GOOD: " << nGoodPats << " patterns recognized, 12s period found." << std::endl;
      } else if(nGoodPats > 3) {
	isRS = 1;
	printf("  RS GOOD: %d patterns recognized, 12s period not found.\n" , nGoodPats);
	oFile << "  RS GOOD: " << nGoodPats << " patterns recognized, 12s period not found." << std::endl;
      } else if(foundLong == 1) {
	isRS = 0;
	printf("  RS BAD: %d patterns recognized, 12s period found.\n" , nGoodPats);
	oFile << "  RS BAD: " << nGoodPats << " patterns recognized, 12s period found." << std::endl;
      } else {
	isRS = 0;
	printf("  RS BAD: %d patterns recognized, 12s period not found.\n" , nGoodPats);
	oFile << "  RS BAD: " << nGoodPats << " patterns recognized, 12s period not found." << std::endl;
      }
    }


    //Conclusions
    if(isRS == isBS) {
      if(isRS == 1) {
	printf("Radar signal and Blanking Signal present.  Continuing to next Segment...\n");
      } else {
	printf("Neither Radar signal nor Blanking signal present.  Continuing to next Segment...\n");
      }
    } else {
      if(isRS == 1) {
	printf("Radar signal present, and no blanking signal.  Generating Blanking Signal...\n");
      } else {
	printf("Radar signal absent, and blanking signal present.  Removing Blanking Signal...\n");
      } 
    }
    // * * * *
    // * * * * * * * * Generate Blanking Signal * * * * * * * *
    // * * * *
    
    int rsType1, rsType2;
    long long startBSLoc;
    // * Figure out type of Radio Signal
    for(int i = 5; i < size; i++) {
      rsType1 = 0; //alternating periods
      rsType2 = 0; //constant period
      for(int j = 5; j > 0; j--) {
	if(sumSig[3][i-j] == j) {
	  rsType1++;
	} else if(sumSig[3][i-j] == 6) {
	  rsType2++;
	}	  
      }
      if(rsType1 == 5) {
	if(rsType2 == 5) {
	  printf("ERROR:  Both RS types detected.\n");
	}	   
	i = size;
	rsType2 = 0;
	startBSLoc = sumSig[0][i-5];
	printf("Type 1 Radar signal detected and locked.\n");
      } else if(rsType2 == 5) {
	if(rsType1 == 5) {
	  printf("ERROR:  Both RS types detected.\n");
	}
	i = size;
	rsType1 = 0;
	startBSLoc = sumSig[0][i-5];
	printf("Type 2 Radar signal detected and locked.\n");
      } else {
	rsType1 = 0;
	rsType2 = 0;
      }
    } //for i < size
    

    oFile.flush();
    largestSums.clear();
    largestSumsLoc.clear();
    sumSig[0].clear();
    sumSig[1].clear();
    blankSig.clear();
    genBS.clear();
    if( !(iter < stop-1 || stop == 0) ){
      printf("reached stop point.\n");
    }

    
  } // while iter
  oFile << "Procession Complete.";
  oFile.close();
  printf("Printing Complete. \n\n");
  
}
  
