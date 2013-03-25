#include <iostream>
#include <iomanip>
#include "setilib.h"

// In the following mapping, the comment terms are:
// 	Input:    the actual hardware inputs on the front
//           	  panel of the front end electronics.
// 	EDT Bits: the bit position in a 16 bit data word.
// 	Channel:  a pair of bits, eg bits 0,1 -> channel 0
//
// Note that the EDT Bits repeat themselves, eg:
// 	Input  6, EDT Bits 2,3,   Channel 1
// 	Input 14, EDT Bits 2,3,   Channel 9
// This is because the data are acquired on 2 EDT cards.
// Inputs 1-8 (channels 2,3,6,7,0,1,4,5) are on EDT
// card 1, while Inputs 9-15 (channels 10,11,14,15,*,9,12,*)
// are on EDT card 0 (note that Inputs 13 and 16 are unused).
//
// The fact that the low order input set is on card 1 while
// the high order input set is on card 0 is due to the way
// the frontend electronics is cabled to the linux host (in
// which sit the EDT cards).  If one would swap these cables
// the Input set to EDT card relationship would reverse itself.
//
// In the SW, there is reference to "DSI".  This stands for
// Data Stream Index and equates to EDT card number (DSI 0 =
// EDT card 0).
//
// Getting to the correct data stream for the inputs you want
// is handled in seti_GetDr2Data().  Data stream is not encoded
// in any way in the following mapping.
//
// There is a further complication.  The data are acquired
// on an Intel machine which has a little endian byte order.  
// Data may (or may not) occur on a big endian byte order machine.
// If so, bits 0,1,2,3,4,5,6,7 swap places with bits 8,9,10,11,
// 12,13,14,15 within a word.  This complication is hidden by
// a set of processor detecting data access classes.

const int       blanking_input = 16;  // HW input which carries the 
                                      // (radar) blanking signal
const short     blanking_bit = SOFTWARE_BLANKING_BIT; // Which of the 2 bits in the blanking_input
                                              // carries the signal.  Possible values: 1 or 2.
                                              // 1 - software blanking bit 
                                              // 2 - hardware blanking bit
                                              
static const std::pair<telescope_id,channel_t> rec2ch_init[14]={
                                                      // DSI 0
  std::pair<telescope_id,channel_t>(AO_ALFA_0_0, 2),  // Input  1, EDT Bits 4,5,   Channel 2
  std::pair<telescope_id,channel_t>(AO_ALFA_0_1, 3),  // Input  2, EDT Bits 6,7,   Channel 3
  std::pair<telescope_id,channel_t>(AO_ALFA_1_0, 6),  // Input  3, EDT Bits 12,13, Channel 6
  std::pair<telescope_id,channel_t>(AO_ALFA_1_1, 7),  // Input  4, EDT Bits 14,15, Channel 7
  std::pair<telescope_id,channel_t>(AO_ALFA_2_0, 0),  // Input  5, EDT Bits 0,1,   Channel 0
  std::pair<telescope_id,channel_t>(AO_ALFA_2_1, 1),  // Input  6, EDT Bits 2,3,   Channel 1
  std::pair<telescope_id,channel_t>(AO_ALFA_3_0, 4),  // Input  7, EDT Bits 8,9,   Channel 4
  std::pair<telescope_id,channel_t>(AO_ALFA_3_1, 5),  // Input  8, EDT Bits 10,11  Channel 5
                                                      // DSI 1
  std::pair<telescope_id,channel_t>(AO_ALFA_4_0, 10), // Input  9, EDT Bits 4,5,   Channel 10
  std::pair<telescope_id,channel_t>(AO_ALFA_4_1, 11), // Input 10, EDT Bits 6,7    Channel 11
  std::pair<telescope_id,channel_t>(AO_ALFA_5_0, 14), // Input 11, EDT Bits 12,13, Channel 14
  std::pair<telescope_id,channel_t>(AO_ALFA_5_1, 15), // Input 12, EDT Bits 14,15, Channel 15
                                                      // Input 13, EDT Bits 0,1,   NOT USED
  std::pair<telescope_id,channel_t>(AO_ALFA_6_0, 9),  // Input 14, EDT Bits 2,3,   Channel 9
  std::pair<telescope_id,channel_t>(AO_ALFA_6_1, 12)  // Input 15, EDT Bits 8,9,   Channel 12
                                                      // Input 16, EDT Bits 10,11, Blanking 
};

static const std::pair<channel_t,telescope_id> ch2rec_init[14]={
                                                       // DSI 0
  std::pair<channel_t,telescope_id>(2,  AO_ALFA_0_0),  // Input  1, EDT Bits 4,5,   Channel 2
  std::pair<channel_t,telescope_id>(3,  AO_ALFA_0_1),  // Input  2, EDT Bits 6,7,   Channel 3
  std::pair<channel_t,telescope_id>(6,  AO_ALFA_1_0),  // Input  3, EDT Bits 12,13, Channel 6
  std::pair<channel_t,telescope_id>(7,  AO_ALFA_1_1),  // Input  4, EDT Bits 14,15, Channel 7
  std::pair<channel_t,telescope_id>(0,  AO_ALFA_2_0),  // Input  5, EDT Bits 0,1,   Channel 0
  std::pair<channel_t,telescope_id>(1,  AO_ALFA_2_1),  // Input  6, EDT Bits 2,3,   Channel 1
  std::pair<channel_t,telescope_id>(4,  AO_ALFA_3_0),  // Input  7, EDT Bits 8,9,   Channel 4
  std::pair<channel_t,telescope_id>(5,  AO_ALFA_3_1),  // Input  8, EDT Bits 10,11  Channel 5
                                                       // DSI 1
  std::pair<channel_t,telescope_id>(10, AO_ALFA_4_0),  // Input  9, EDT Bits 4,5,   Channel 10
  std::pair<channel_t,telescope_id>(11, AO_ALFA_4_1),  // Input 10, EDT Bits 6,7    Channel 11
  std::pair<channel_t,telescope_id>(14, AO_ALFA_5_0),  // Input 11, EDT Bits 12,13, Channel 14
  std::pair<channel_t,telescope_id>(15, AO_ALFA_5_1),  // Input 12, EDT Bits 14,15, Channel 15
                                                       // Input 13, EDT Bits 0,1,   NOT USED
  std::pair<channel_t,telescope_id>(9,  AO_ALFA_6_0),  // Input 14, EDT Bits 2,3,   Channel 9
  std::pair<channel_t,telescope_id>(12, AO_ALFA_6_1)   // Input 15, EDT Bits 8,9,   Channel 12
                                                       // Input 16, EDT Bits 10,11, Blanking 
};

static const std::pair<channel_t,int> ch2vgci_init[14]={
                                            // DSI 0
  std::pair<channel_t,int>(2,  0),          // AO_ALFA_0_0, Input  1, EDT Bits 4,5,   Channel 2  
  std::pair<channel_t,int>(3,  1),          // AO_ALFA_0_1, Input  2, EDT Bits 6,7,   Channel 3
  std::pair<channel_t,int>(6,  2),          // AO_ALFA_1_0, Input  3, EDT Bits 12,13, Channel 6
  std::pair<channel_t,int>(7,  3),          // AO_ALFA_1_1, Input  4, EDT Bits 14,15, Channel 7
  std::pair<channel_t,int>(0,  4),          // AO_ALFA_2_0, Input  5, EDT Bits 0,1,   Channel 0
  std::pair<channel_t,int>(1,  5),          // AO_ALFA_2_1, Input  6, EDT Bits 2,3,   Channel 1
  std::pair<channel_t,int>(4,  6),          // AO_ALFA_3_0, Input  7, EDT Bits 8,9,   Channel 4
  std::pair<channel_t,int>(5,  7),          // AO_ALFA_3_1, Input  8, EDT Bits 10,11  Channel 5
                                            // DSI 1
  std::pair<channel_t,int>(10, 0),          // AO_ALFA_4_0, Input  9, EDT Bits 4,5,   Channel 10   
  std::pair<channel_t,int>(11, 1),          // AO_ALFA_4_1, Input 10, EDT Bits 6,7    Channel 11    
  std::pair<channel_t,int>(14, 2),          // AO_ALFA_5_0, Input 11, EDT Bits 12,13, Channel 14  
  std::pair<channel_t,int>(15, 3),          // AO_ALFA_5_1, Input 12, EDT Bits 14,15, Channel 15 
                                            //              Input 13, EDT Bits 0,1,   NOT USED
  std::pair<channel_t,int>(9,  5),          // AO_ALFA_6_0, Input 14, EDT Bits 2,3,   Channel 9
  std::pair<channel_t,int>(12, 6)           // AO_ALFA_6_1, Input 15, EDT Bits 8,9,   Channel 12
                                            //              Input 16, EDT Bits 10,11, Blanking 
};


typedef std::pair<int,int> bmpol_t;

static const std::pair<bmpol_t,channel_t> bmpol2ch_init[14]={
                                                  // DSI 0
  std::pair<bmpol_t,channel_t>(bmpol_t(0,0), 2),  // Input  1, EDT Bits 4,5,   Channel 2
  std::pair<bmpol_t,channel_t>(bmpol_t(0,1), 3),  // Input  2, EDT Bits 6,7,   Channel 3
  std::pair<bmpol_t,channel_t>(bmpol_t(1,0), 6),  // Input  3, EDT Bits 12,13, Channel 6
  std::pair<bmpol_t,channel_t>(bmpol_t(1,1), 7),  // Input  4, EDT Bits 14,15, Channel 7
  std::pair<bmpol_t,channel_t>(bmpol_t(2,0), 0),  // Input  5, EDT Bits 0,1,   Channel 0
  std::pair<bmpol_t,channel_t>(bmpol_t(2,1), 1),  // Input  6, EDT Bits 2,3,   Channel 1
  std::pair<bmpol_t,channel_t>(bmpol_t(3,0), 4),  // Input  7, EDT Bits 8,9,   Channel 4
  std::pair<bmpol_t,channel_t>(bmpol_t(3,1), 5),  // Input  8, EDT Bits 10,11  Channel 5
                                                  // DSI 1
  std::pair<bmpol_t,channel_t>(bmpol_t(4,0), 10), // Input  9, EDT Bits 4,5,   Channel 10
  std::pair<bmpol_t,channel_t>(bmpol_t(4,1), 11), // Input 10, EDT Bits 6,7    Channel 11
  std::pair<bmpol_t,channel_t>(bmpol_t(5,0), 14), // Input 11, EDT Bits 12,13, Channel 14
  std::pair<bmpol_t,channel_t>(bmpol_t(5,1), 15), // Input 12, EDT Bits 14,15, Channel 15
                                                  // Input 13, EDT Bits 0,1,   NOT USED
  std::pair<bmpol_t,channel_t>(bmpol_t(6,0), 9),  // Input 14, EDT Bits 2,3,   Channel 9
  std::pair<bmpol_t,channel_t>(bmpol_t(6,1), 12)  // Input 15, EDT Bits 8,9,   Channel 12
                                                  // Input 16, EDT Bits 10,11, Blanking 
};

// Note that we map all 16 inputs here. At times will will
// want to ask for "unused" inputs as they will be serving
// some special purpose.
static const std::pair<input_t,channel_t> input2ch_init[16]={
                                            // DSI 0
  std::pair<input_t,channel_t>(1, 2),       // Input  1, EDT Bits 4,5,   Channel 2
  std::pair<input_t,channel_t>(2, 3),       // Input  2, EDT Bits 6,7,   Channel 3
  std::pair<input_t,channel_t>(3, 6),       // Input  3, EDT Bits 12,13, Channel 6
  std::pair<input_t,channel_t>(4, 7),       // Input  4, EDT Bits 14,15, Channel 7
  std::pair<input_t,channel_t>(5, 0),       // Input  5, EDT Bits 0,1,   Channel 0
  std::pair<input_t,channel_t>(6, 1),       // Input  6, EDT Bits 2,3,   Channel 1
  std::pair<input_t,channel_t>(7, 4),       // Input  7, EDT Bits 8,9,   Channel 4
  std::pair<input_t,channel_t>(8, 5),       // Input  8, EDT Bits 10,11, Channel 5
                                            // DSI 1
  std::pair<input_t,channel_t>(9, 10),      // Input  9, EDT Bits 4,5,   Channel 10
  std::pair<input_t,channel_t>(10, 11),     // Input 10, EDT Bits 6,7,   Channel 11
  std::pair<input_t,channel_t>(11, 14),     // Input 11, EDT Bits 12,13, Channel 14
  std::pair<input_t,channel_t>(12, 15),     // Input 12, EDT Bits 14,15, Channel 15
  std::pair<input_t,channel_t>(13, 8),      // Input 13, EDT Bits 0,1,   Channel 8
  std::pair<input_t,channel_t>(14, 9),      // Input 14, EDT Bits 2,3,   Channel 9
  std::pair<input_t,channel_t>(15, 12),     // Input 15, EDT Bits 8,9,   Channel 12
  std::pair<input_t,channel_t>(16, 13)      // Input 16, EDT Bits 10,11, Channel 13


};

  
std::map<telescope_id,channel_t> receiverid_to_channel(rec2ch_init,rec2ch_init+14);

std::map<channel_t,telescope_id> channel_to_receiverid(ch2rec_init,ch2rec_init+14);

std::map<channel_t,int> channel_to_vgci(ch2vgci_init,ch2vgci_init+14);   // channel to VGC index

std::map<bmpol_t,channel_t> beam_pol_to_channel(bmpol2ch_init,bmpol2ch_init+14);

std::map<input_t,channel_t> input_to_channel(input2ch_init,input2ch_init+16);  // all 16 here

//------------------------------------------------------------
int vgci_by_channel(channel_t channel, dataheader_t& header) {
//------------------------------------------------------------

    int dsi = channel > 7 ? 0 : 1;  // this should be a callable method
    // fprintf(stderr, "dsi : %d\n", dsi);

    if(dsi != header.dsi) {
        return(-1);         // error : this header does not contain the vgc for this channel
    } else {
        std::map<channel_t,int>::iterator pos = channel_to_vgci.find(channel);
        if(pos == channel_to_vgci.end()) {
            return(-1);     // error : channel not in map  
        } else {
            return((*pos).second);
        }
    }
}

//------------------------------------------------------------
int get_vgc_for_channel(channel_t channel, dataheader_t& header) {
//------------------------------------------------------------

    int vgci = vgci_by_channel(channel, header);

    // turns out channel should already be in header (as part of GetDr2Data
    // populating the derived values when obtaining the header in the first
    // place). If so, we can remove channel as a passed variable to this
    // function. Until then, warn if they don't match, just to be sure this
    // assumption is correct...
    if (channel != header.channel)
      fprintf(stderr,"Warning: passed channel %d does not match channel in header: %d\n",channel,header.channel);

    if(vgci < 0) {
        return(vgci);       // error
    } else {
        // fprintf(stderr, "vgci : %d\n", vgci);
        return(header.vgc[vgci]);
    }
}

//#define DEBUG

//------------------------------------------------------------
void dataheader_t::populate_from_data (char * header) {
//------------------------------------------------------------

    char headercopy[HeaderSize];
    char * header_p;
    std::vector<char *> words;

    header_p = (char *)memcpy(headercopy,header,HeaderSize);

    do {
        words.clear();                                  // (re)init vectorized line

        // parse 1 line into a vector of words
        while(*header_p != '\n') {                      // while not EOL

            // remove any leading white spaces and nulls
            while((isspace(*header_p) || *header_p == '\0') && *header_p != '\n') {
                header_p++;
            }

            // if not EOL then we have a word
            if(*header_p != '\n') {
                words.push_back(header_p);              // word address to vector
                while(!isspace(*header_p) && *header_p != '\0') {
                    header_p++;                         // go one past the word
                }
            }
            if(*header_p != '\n') *header_p = '\0';     // terminate the word

        }   // end while not EOL

        *header_p++ = '\0';                             // term. final word and position to next line   

        parse_line(words);                             // parse this line
        
    } while (strcmp(words[0], "END_OF_HEADER") != 0);   // header complete

}

//------------------------------------------------------------
void dataheader_t::populate_derived_values(channel_t this_channel) {
//------------------------------------------------------------

    channel = this_channel;
    double coord_unixtime = seti_ao_timeMS2unixtime(scram_agc.Time, scram_agc.SysTime);

    if(scram_agc.SysTime) {
        
        seti_time temp_time((time_t)0,coord_unixtime); 
        coord_time = temp_time; 

	    double Za        = scram_agc.Za;    // don't change the header since we want to keep it as read.
	    double Az        = scram_agc.Az;    // don't change the header since we want to keep it as read.
        double Rotation  = scram_alfashm.AlfaMotorPosition;
        telescope_id Tel = channel_to_receiverid[channel];

        co_ZenAzCorrection(Tel, &Za, &Az, Rotation);       
//fprintf(stderr, "TIME %ld %ld %lf\n", scram_agc.Time, scram_agc.SysTime, seti_ao_timeMS2unixtime(scram_agc.Time, scram_agc.SysTime));
        seti_AzZaToRaDec(Az, Za, coord_unixtime, ra, dec);
    } else {
        fprintf(stderr, "In populate_derived_values() : no scram_agc.SysTime!\n");
        exit(1);
    }
}
    

//------------------------------------------------------------
int dataheader_t::parse_line(std::vector<char *> &words) {
//------------------------------------------------------------

    std::vector<char *>::iterator word, printword;

#ifdef DEBUG
    for(printword = words.begin(); printword < words.end(); printword++) {
        std::cout << *printword << " ";
    }
    std::cout << std::endl;
#endif

    for(word = words.begin(); word < words.end(); word++) {

        if(strcmp(*word, "HEADER_SIZE") == 0) {
            header_size = atoi(*(++word));
        }
        if(strcmp(*word, "DATA_SIZE") == 0) {
            data_size = atoi(*(++word));
        }
        if(strcmp(*word, "NAME") == 0) {
            strcpy(name, (*(++word)));
        }
        if(strcmp(*word, "DSI") == 0) {
            dsi = atoi(*(++word));
        }
        if(strcmp(*word, "FRAMESEQ") == 0) {
            frameseq = atoi(*(++word));
        }
        if(strcmp(*word, "DATASEQ") == 0) {
            dataseq = atoi(*(++word));
        }
        if(strcmp(*word, "IDLECOUNT") == 0) {
            idlecount = atoi(*(++word));
        }
        if(strcmp(*word, "MISSED") == 0) {
            missed = atoi(*(++word));
        }
        if(strcmp(*word, "AST") == 0) {
            //strcpy(ast, (*(++word)));
            seti_time temp_time((*(++word)));
            //ast = temp_time;
            data_time = temp_time;
        }
        if(strcmp(*word, "SYNTH") == 0) {
            //strcpy(synth_time, (*(++word)));
            seti_time temp_time((*(++word)));
            synth_time = temp_time;
            synth_freq = atoi(*(++word));
            sky_freq   = atoi(*(++word));
        }
        if(strcmp(*word, "ENG") == 0) {
            int i;
            for(i=0; i<8; i++) sscanf((*(++word)), "%*d:%d", &vgc[i]);
        }
        if(strcmp(*word, "SAMPLERATE") == 0) {
            samplerate = atof(*(++word));
        }
        if(strcmp(*word, "SCRAM") == 0) {
            word++;
            if(strcmp(*word, "AGC") == 0) {
                while(++word < words.end()) {
                    if(strcmp(*word, "AGC_SysTime") == 0) {
                        scram_agc.SysTime = atoi(*(++word));
                    }
                    if(strcmp(*word, "AGC_Az") == 0) {
                        scram_agc.Az = atof(*(++word));
                    }
                    if(strcmp(*word, "AGC_Za") == 0) {
                        scram_agc.Za = atof(*(++word));
                    }
                    if(strcmp(*word, "AGC_Time") == 0) {
                        scram_agc.Time = atoi(*(++word));
                    }
                }
		return 0;
            }   // end AGC
            if(strcmp(*word, "ALFASHM") == 0) {
                while(++word < words.end()) {
                    if(strcmp(*word, "ALFASHM_SysTime") == 0) {
                        scram_alfashm.SysTime = atoi(*(++word));
                    }
                    if(strcmp(*word, "ALFASHM_AlfaFirstBias") == 0) {
                        scram_alfashm.AlfaFirstBias = atoi(*(++word));
                    }
                    if(strcmp(*word, "ALFASHM_AlfaSecondBias") == 0) {
                        scram_alfashm.AlfaSecondBias = atoi(*(++word));
                    }
                    if(strcmp(*word, "ALFASHM_AlfaMotorPosition") == 0) {
                        scram_alfashm.AlfaMotorPosition = atof(*(++word));
                    }
                }
		return 0;
            }   // end ALFASHM
            if(strcmp(*word, "IF1") == 0) {
                while(++word < words.end()) {
                    if(strcmp(*word, "IF1_SysTime") == 0) {
                        scram_if1.SysTime = atoi(*(++word));
                    }
                    if(strcmp(*word, "IF1_synI_freqHz_0") == 0) {
                        scram_if1.synI_freqHz_0 = atof(*(++word));
                    }
                    if(strcmp(*word, "IF1_synI_ampDB_0") == 0) {
                        scram_if1.synI_ampDB_0 = atoi(*(++word));
                    }
                    if(strcmp(*word, "IF1_rfFreq") == 0) {
                        scram_if1.rfFreq = atof(*(++word));
                    }
                    if(strcmp(*word, "IF1_if1FrqMhz") == 0) {
                        scram_if1.if1FrqMhz = atof(*(++word));
                    }
                }
		return 0;
            }   // end IF1
            if(strcmp(*word, "IF2") == 0) {
                while(++word < words.end()) {
                    if(strcmp(*word, "IF2_SysTime") == 0) {                         
                        scram_if2.SysTime = atoi(*(++word));
                    }
                    if(strcmp(*word, "IF2_useAlfa") == 0) {
                        scram_if2.useAlfa = atoi(*(++word));
                    }
                }
		return 0;
            }   // end IF2
            if(strcmp(*word, "TT") == 0) {
                while(++word < words.end()) {
                    if(strcmp(*word, "TT_SysTime") == 0) {
                        scram_tt.SysTime = atoi(*(++word));
                    }
                    if(strcmp(*word, "TT_TurretEncoder") == 0) {
                        scram_tt.TurretEncoder = atoi(*(++word));
                    }
                    if(strcmp(*word, "TT_TurretDegrees") == 0) {
                        scram_tt.TurretDegrees = atof(*(++word));
                    }
                }
		return 0;
            }   // end TT
        }   // end SCRAM
    }  // end loop through words vector

    return(0);
}


//------------------------------------------------------------
void dataheader_t::print() {
//------------------------------------------------------------

    fprintf(stderr, " header_size     %d\n data_size       %d\n name            %s\n dsi             %d\n channel         %d\n frameseq        %ld\n dataseq         %ld\n idlecount       %ld\n missed          %d\n synth_freq      %ld\n sky_freq        %ld\n VGCs            0:%d 1:%d 2:%d 3:%d 4:%d 5:%d 6:%d 7:%d\n ra        %lf\n dec       %lf\n coord_time       %lf\n data_time       %lf\n samplerate      %lf\n scram_agc.Az    %lf\n scram_agc.Za    %lf\n scram_agc.Time  %lu\n scram_agc.SysTime   %lu\n scram_alfashm.AlfaFirstBias %d\n scram_alfashm.AlfaSecondBias    %d\n scram_alfashm.AlfaMotorPosition %f\n scram_alfashm.SysTime   %lu\n scram_if1.synI_freqHz_0 %lf\n scram_if1.synI_ampDB_0  %d\n scram_if1.rfFreq    %lf\n scram_if1.if1FrqMhz %lf\n scram_if1.alfaFb    %d\n scram_if1.SysTime   %lu\n scram_if2.useAlfa   %d\n scram_if2.SysTime   %lu\n scram_tt.TurretEncoder  %d\n scram_tt.TurretDegrees  %lf\n scram_tt.TurretDegreesAlfa  %lf\n scram_tt.TurretDegreesTolerance %lf\n scram_tt.SysTime    %lu\n scram_pnt.Ra    %lf\n scram_pnt.Dec   %lf\n scram_pnt.MJD   %lf\n scram_pnt.SysTime   %lu\n blanking_failed  %d\n",
            header_size,
            data_size, 
            name, 
            dsi, 
            channel,
            frameseq, 
            dataseq, 
            idlecount, 
            missed,
            //0,
            //0,
            //ast, 
            //synth_time,
            synth_freq,
            sky_freq,
            vgc[0], vgc[1], vgc[2], vgc[3], vgc[4], vgc[5], vgc[6], vgc[7],
            ra, 
            dec, 
            0.0,
            //coord_jd,
            0.0,
            //data_jd,
            samplerate,
            scram_agc.Az,    
            scram_agc.Za,    
            scram_agc.Time,
            scram_agc.SysTime,
            scram_alfashm.AlfaFirstBias,
            scram_alfashm.AlfaSecondBias,
            scram_alfashm.AlfaMotorPosition,
            scram_alfashm.SysTime,
            scram_if1.synI_freqHz_0, 
            scram_if1.synI_ampDB_0,
            scram_if1.rfFreq,    
            scram_if1.if1FrqMhz, 
            scram_if1.alfaFb,
            scram_if1.SysTime,
            scram_if2.useAlfa,
            scram_if2.SysTime,
            scram_tt.TurretEncoder,
            scram_tt.TurretDegrees,  
            scram_tt.TurretDegreesAlfa,  
            scram_tt.TurretDegreesTolerance, 
            scram_tt.SysTime,
            scram_pnt.Ra,
            scram_pnt.Dec,
            scram_pnt.MJD,
            scram_pnt.SysTime,
            blanking_failed
    );
}

//------------------------------------------------------------
dataheader_t::dataheader_t() :
//------------------------------------------------------------
    header_size(0),
    data_size(0),
    dsi(0),
    channel(0),
    frameseq(0),
    dataseq(0),
    idlecount(0),
    missed(0),
    //agc_systime(0),
    ra(0),
    dec(0),
    //coord_jd(0),
    //data_jd(0),
    //centerfreq(0),
    samplerate(0), 
    blanking_failed(false) {

    strcpy(name, "\0");
    //strcpy(ast, "\0");

    scram_agc.Az = 0;
    scram_agc.Za = 0;
    scram_agc.Time = 0;
    scram_agc.SysTime = 0;

    scram_alfashm.AlfaFirstBias = 0; 
    scram_alfashm.AlfaSecondBias = 0; 
    scram_alfashm.AlfaMotorPosition = 0; 
    scram_alfashm.SysTime = 0; 

    scram_if1.synI_freqHz_0 = 0;
    scram_if1.synI_ampDB_0 = 0;
    scram_if1.rfFreq = 0;
    scram_if1.if1FrqMhz = 0;
    scram_if1.alfaFb = 0;
    scram_if1.SysTime = 0;

    scram_if2.useAlfa = 0;
    scram_if2.SysTime = 0;

    scram_tt.TurretEncoder = 0;
    scram_tt.TurretDegrees = 0;
    scram_tt.TurretDegreesAlfa = 0;
    scram_tt.TurretDegreesTolerance = 0;
    scram_tt.SysTime = 0;

    scram_pnt.Ra = 0;
    scram_pnt.Dec = 0;
    scram_pnt.MJD = 0;
    scram_pnt.SysTime = 0;
}

#if 0
//------------------------------------------------------------
int seti_GetDr2CommonWaveform(int fd, int dsi, ssize_t numblocks,
//------------------------------------------------------------
  std::vector<dr2_block_t> &dr2_block) {

    dr2_block_t                  this_dr2_block;
    std::vector<complex<float> > common_waveform;
    int                          blockcount=0;

    dr2_block.reserve(numblocks+dr2_block.size());
    channel_t channel=(dsi?8:0);

    while (blockcount < numblocks) {
        this_dr2_block.data.clear();
	    common_waveform.clear();
        if(seti_GetDr2Data(fd, 
                           channel, 
                           DataWords, 
                           this_dr2_block.data,
	                       &this_dr2_block.header, 
                           common_waveform
                          ) != (int)DataWords) {
             break;
        }
	    this_dr2_block.data=common_waveform;
        blockcount++;
        dr2_block.push_back(this_dr2_block);
    }


    return(blockcount);
}
#endif

//------------------------------------------------------------
int seti_StructureDr2Data(int fd, channel_t channel, ssize_t numblocks,
  std::vector<dr2_block_t> &dr2_block, blanking_filter<complex<float> > filter) {
//------------------------------------------------------------

    dr2_block_t                     this_dr2_block;
    std::vector<int>                blanking_signal;
    int                             blockcount=0;
    bool                            blanking;

    dr2_block.reserve(numblocks+dr2_block.size());

    if (filter) {
      blanking=true;
    } else {
      blanking=false;
    }

    while (blockcount < numblocks) {
        this_dr2_block.data.clear();
	    blanking_signal.clear();
        if(seti_GetDr2Data(fd, 
                           channel,         
                           DataWords, 
                           this_dr2_block.data,
	                       &this_dr2_block.header, 
                           blanking_signal,
                           blanking
                          ) != (int)DataWords) {
             break;
        }
        blockcount++;
	if (filter) filter(this_dr2_block.data,blanking_signal);
        dr2_block.push_back(this_dr2_block);
    }


    return(blockcount);
}

//------------------------------------------------------------
int seti_StructureDr2Data(int fd, telescope_id receiver, ssize_t numblocks,
std::vector<dr2_block_t> &dr2_block, blanking_filter<complex<float> > filter) {
//------------------------------------------------------------
    return
    seti_StructureDr2Data(fd,receiverid_to_channel[receiver],numblocks,dr2_block,filter);
}

//------------------------------------------------------------
int seti_StructureDr2Data(int fd, int beam, int pol, ssize_t numblocks,
std::vector<dr2_block_t> &dr2_block, blanking_filter<complex<float> > filter) {
//------------------------------------------------------------
    return
    seti_StructureDr2Data(fd,beam_pol_to_channel[bmpol_t(beam,pol)],numblocks,dr2_block,filter);
}

//------------------------------------------------------------
int seti_StructureDr2Data(int fd, channel_t channel, ssize_t numblocks,
std::vector<dr2_compact_block_t> &dr2_block, blanking_filter<complex<signed
char> > filter) {
//------------------------------------------------------------

    dr2_compact_block_t                 this_dr2_block;
    std::vector<int>                    blanking_signal;
    int                                 blockcount=0;
    bool                                blanking;

    dr2_block.reserve(numblocks+dr2_block.size());

    if (filter) {
      blanking=true;
    } else {
      blanking=false;
    }

    while (blockcount < numblocks) {
        this_dr2_block.data.clear();
	    blanking_signal.clear();
        if(seti_GetDr2Data(fd, 
                           channel, 
                           DataWords, 
                           this_dr2_block.data,
	                       &this_dr2_block.header,
                           blanking_signal,
                           blanking
                          ) != (int)DataWords) {
             break;
        }
        blockcount++;
	if (filter) filter(this_dr2_block.data,blanking_signal);
    dr2_block.push_back(this_dr2_block);
    }

    return(blockcount);
}

//------------------------------------------------------------
int seti_StructureDr2Data(int fd, telescope_id receiver, ssize_t numblocks,
std::vector<dr2_compact_block_t> &dr2_block, blanking_filter<complex<signed char> > filter) {
//------------------------------------------------------------
    return
    seti_StructureDr2Data(fd,receiverid_to_channel[receiver],numblocks,dr2_block,filter);
}

//------------------------------------------------------------
int seti_StructureDr2Data(int fd, int beam, int pol, ssize_t numblocks,
std::vector<dr2_compact_block_t> &dr2_block, blanking_filter<complex<signed
char> > filter) {
//------------------------------------------------------------
    return
    seti_StructureDr2Data(fd,beam_pol_to_channel[bmpol_t(beam,pol)],numblocks,dr2_block,filter);
}

//------------------------------------------------------------
bool seti_FindAdjacentDr2Buffer(int fd, dataheader_t &dataheader, off_t &adjacent_buffer_file_pos) {
//------------------------------------------------------------
// This routine will, on success, return the file position of the sequence-adjacent buffer in the *opposite* dsi 
//#define DEBUG_FindAdjacentDr2Buffer

    bool found=false;

    dataheader_t this_header;

    off_t offset=(off_t)BufSize;        // always starts positive, may flip to negative

    off_t seek_save;
    seek_save = lseek(fd, 0, SEEK_CUR);

    // on entry, new_seek is our best guess starting point
    // simple, inefficient on first entry, then every dr2BlockSize entries (dsi crossover)
    static off_t new_seek;              
    static bool  first_entry=true;           
    if(first_entry) {                
        first_entry = false;         
        new_seek = seek_save;    
    }

    #ifdef DEBUG_FindAdjacentDr2Buffer
    fprintf(stderr, "FindAdjacentDr2Buffer : starting at file position %lu\n", seek_save);
    #endif
    while (1) {      // break on success, error, or exhausted search
        new_seek += offset;

        if ((lseek(fd, new_seek, SEEK_SET) == -1)) {
            #ifdef DEBUG_FindAdjacentDr2Buffer
            fprintf(stderr, "FindAdjacentDr2Buffer : lseek error (could be EOF)\n");
            #endif
            break;                                              // error - cannot lseek 
        }

        if (!seti_PeekDr2Header(fd, this_header)) {
            #ifdef DEBUG_FindAdjacentDr2Buffer
            fprintf(stderr, "FindAdjacentDr2Buffer : cannot peek at header)\n");
            #endif
            break;                                              // error - cannot get header 
        }
        if (this_header.dsi == dataheader.dsi) {                // not in a block with opposite dsi
           continue;
        } 

        // we are now in a block with the opposite dsi.  There are 4 possibilities here.
        if (this_header.dataseq == dataheader.dataseq) {                        // (1) we found the adj. buffer
            adjacent_buffer_file_pos = lseek(fd, 0, SEEK_CUR);
            found = true;
            #ifdef DEBUG_FindAdjacentDr2Buffer
            fprintf(stderr, "FindAdjacentDr2Buffer at new_seek %lu (%d %d %d %d) : found adjacent buffer.\n",
                    new_seek, dataheader.dsi, dataheader.dataseq, this_header.dsi, this_header.dataseq);
            fprintf(stderr, "times : %21.12lf %21.12lf %21.12lf \n", 
                    this_header.data_time.jd().uval(), 
                    dataheader.data_time.jd().uval(), 
                    fabs(this_header.data_time.jd().uval() - dataheader.data_time.jd().uval()));
            #endif
            break;                                            
        } else if((offset > 0 && this_header.dataseq < dataheader.dataseq) ||
                  (offset < 0 && this_header.dataseq > dataheader.dataseq)) {   // (2) we are moving in the right direction
            #ifdef DEBUG_FindAdjacentDr2Buffer
            fprintf(stderr, "FindAdjacentDr2Buffer at new_seek %lu (%d %d %d %d) : keeping offset at %ld\n",
                    new_seek, dataheader.dsi, dataheader.dataseq, this_header.dsi, this_header.dataseq, offset);
            #endif
            continue;                                             
        } else if((offset > 0 && this_header.dataseq > dataheader.dataseq) ||
                  (offset < 0 && this_header.dataseq < dataheader.dataseq)) {   // (3) we are moving in the wrong direction
            if(offset > 0) {                                                    // ... but can still change directions
                offset = -offset;
            #ifdef DEBUG_FindAdjacentDr2Buffer
                fprintf(stderr, "FindAdjacentDr2Buffer at new_seek %lu  (%d %d %d %d) : changing offset to %ld\n",
                        new_seek, dataheader.dsi, dataheader.dataseq, this_header.dsi, this_header.dataseq, offset);
            #endif
                continue;
            } else {                                                            // (4) exhausted search
            #ifdef DEBUG_FindAdjacentDr2Buffer
                fprintf(stderr, "FindAdjacentDr2Buffer at new_seek %lu  (%d %d %d %d) : search exhausted\n",
                        new_seek, dataheader.dsi, dataheader.dataseq, this_header.dsi, this_header.dataseq);
            #endif
                break;                                                         
            }
        } 
    }

    lseek(fd, seek_save, SEEK_SET);                             // return to seek_cur upon entry

    return (found);
}
        
//------------------------------------------------------------
bool seti_PeekDr2Header(int fd, dataheader_t &this_header) {
//------------------------------------------------------------

    bool retval = true;
    off_t seek_save;

    char *header_buf=(char *)calloc(HeaderSize,1);
    if(!header_buf) {
        fprintf(stderr, "seti_PeekDr2Header calloc failed, exiting...\n"); 
        exit(1);
    }

    seek_save = lseek(fd, 0, SEEK_CUR);
    if (static_cast<unsigned long>(read(fd, header_buf, HeaderSize)) != HeaderSize) {
        retval = false;
    } else {
        this_header.populate_from_data(header_buf);
    }
    lseek(fd, seek_save, SEEK_SET);

    if (header_buf) free(header_buf);
    return (retval);
} 
