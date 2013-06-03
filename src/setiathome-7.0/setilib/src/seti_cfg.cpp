/*
 * Source Code Name  : seti_summary.cc
 * Programmer        : Jeff Cobb
 *                   : Project SERENDIP, University of California, Berkeley CA
 * Compiler          : GNU gcc
 * Summary database routines for serendip 4 offline programs.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <strings.h>
#include <cstring>
#include "setilib.h"


// Globals
char *sReceiverConfigFileName;

const char *sProperReceiverConfigFileName = "ReceiverConfig.tab";

void seti_cfg_GetReceiverConfig(telescope_id ReceiverID, receiver_config *ReceiverConfig) {
    char buf[256];
    sprintf(buf,"where s4_id=%d",ReceiverID);
    ReceiverConfig->fetch(buf);
}



//===========================================================
void seti_cfg_GetReceiverConfig(telescope_id ReceiverID, ReceiverConfig_t *ReceiverConfig)
//===========================================================
{
    char *TestStr;

    bool Done = false;
    bool ReceiverFound = false;

    FILE *ConfigFile = NULL;

    char Key[256];
    char InString[512];

    // Get and check receiver config file from process environment.
    // The path can be anything but must end in the the standard
    // name.
    if ((sReceiverConfigFileName = getenv("S4_RECEIVER_CONFIG")) == NULL) {
        fprintf(stderr, "Could not obtain Receiver Config file name\n");
        fprintf(stderr, "Set the S4_RECEIVER_CONFIG enviromental variable\n");
        exit(1);
    } else if ((TestStr = strstr(sReceiverConfigFileName, sProperReceiverConfigFileName)) == NULL ||
            strcmp(sProperReceiverConfigFileName, TestStr) != 0) {
        fprintf(stderr, "Receiver configuration file must be called %s\n",
                sProperReceiverConfigFileName);
        exit(1);
    }

    if ((ConfigFile = fopen(sReceiverConfigFileName, "r")) == NULL) {
        fprintf(stderr, "Could not open receiver config file %s\n",
                sReceiverConfigFileName);
        exit(1);
    }

    // parse the config file
    while (!Done) {

        // get 1 line
        if ((fgets(InString, 512, ConfigFile)) == NULL) {
            fprintf(stderr, "Bad receiver config file\n");
            exit(1);
        }

        // parse it and test for comment.  Comments separate configurations.
        sscanf(InString, "%s", Key);
        if (Key[0] == '#') {
            if (ReceiverFound) {
                Done = true;
            }
            continue;
        }

        // is this the receiver we want?
        if (strcmp(Key, "REC_ID") == 0) {
            sscanf(InString, "%*s %ld", &ReceiverConfig->ReceiverID);
            if (ReceiverConfig->ReceiverID == ReceiverID) {
                ReceiverFound = true;
                ReceiverConfig->ReceiverID = ReceiverID;
                continue;
            }
        }

        // yup - get the configuration
        if (ReceiverFound) {
            if (strcmp(Key, "REC_NAME") == 0) {
                sscanf(InString, "%*s %s", ReceiverConfig->ReceiverName);
                continue;
            }
            if (strcmp(Key, "REC_BEAMWIDTH") == 0) {
                sscanf(InString, "%*s %lf", &ReceiverConfig->BeamWidth);
                continue;
            }
            if (strcmp(Key, "REC_CENTER_FREQ") == 0) {
                sscanf(InString, "%*s %lf", &ReceiverConfig->CenterFreq);
                continue;
            }
            if (strcmp(Key, "REC_LATITUDE") == 0) {
                sscanf(InString, "%*s %lf", &ReceiverConfig->Latitude);
                continue;
            }
            if (strcmp(Key, "REC_WLONGITUDE") == 0) {
                sscanf(InString, "%*s %lf", &ReceiverConfig->WLongitude);
                continue;
            }
            if (strcmp(Key, "REC_LONGITUDE") == 0) {
                sscanf(InString, "%*s %lf", &ReceiverConfig->Longitude);
                continue;
            }
            if (strcmp(Key, "REC_ELEVATION") == 0) {
                sscanf(InString, "%*s %lf", &ReceiverConfig->Elevation);
                continue;
            }
            if (strcmp(Key, "REC_DIAMETER") == 0) {
                sscanf(InString, "%*s %lf", &ReceiverConfig->Diameter);
                continue;
            }
            if (strcmp(Key, "AZ_ORIENTATION") == 0) {
                sscanf(InString, "%*s %lf", &ReceiverConfig->AzOrientation);
                continue;
            }
            if (strcmp(Key, "ZEN_CORR_COEFF") == 0) {
                sscanf(InString, "%*s %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",
                        &ReceiverConfig->ZenCorrCoeff[0],
                        &ReceiverConfig->ZenCorrCoeff[1],
                        &ReceiverConfig->ZenCorrCoeff[2],
                        &ReceiverConfig->ZenCorrCoeff[3],
                        &ReceiverConfig->ZenCorrCoeff[4],
                        &ReceiverConfig->ZenCorrCoeff[5],
                        &ReceiverConfig->ZenCorrCoeff[6],
                        &ReceiverConfig->ZenCorrCoeff[7],
                        &ReceiverConfig->ZenCorrCoeff[8],
                        &ReceiverConfig->ZenCorrCoeff[9],
                        &ReceiverConfig->ZenCorrCoeff[10],
                        &ReceiverConfig->ZenCorrCoeff[11],
                        &ReceiverConfig->ZenCorrCoeff[12]);
                continue;
            }
            if (strcmp(Key, "AZ_CORR_COEFF") == 0) {
                sscanf(InString, "%*s %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",
                        &ReceiverConfig->AzCorrCoeff[0],
                        &ReceiverConfig->AzCorrCoeff[1],
                        &ReceiverConfig->AzCorrCoeff[2],
                        &ReceiverConfig->AzCorrCoeff[3],
                        &ReceiverConfig->AzCorrCoeff[4],
                        &ReceiverConfig->AzCorrCoeff[5],
                        &ReceiverConfig->AzCorrCoeff[6],
                        &ReceiverConfig->AzCorrCoeff[7],
                        &ReceiverConfig->AzCorrCoeff[8],
                        &ReceiverConfig->AzCorrCoeff[9],
                        &ReceiverConfig->AzCorrCoeff[10],
                        &ReceiverConfig->AzCorrCoeff[11],
                        &ReceiverConfig->AzCorrCoeff[12]);
                continue;
            }
            fprintf(stderr, "Unknown receiver config keyword %s\n", Key);
            exit(1);
        }
    }
    if (ConfigFile) {
        fclose(ConfigFile);
    }
}
