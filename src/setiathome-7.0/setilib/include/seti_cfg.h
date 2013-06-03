//________________________________________________________
// Source Code Name  : seti_cfg.cc
// Programmer        : Donnelly/Cobb et al
//                   : Project SERENDIP, University of California, Berkeley CA
// Compiler          : GNU gcc
//________________________________________________________

// TIDs Telescope ID numbers listed, each number is one byte

// TIDs as given on various command lines

// telescope definitions for database  (this is archaic)

typedef struct {
    long    ReceiverID;
    char    ReceiverName[256];
    double  Latitude;       // degrees
    double  WLongitude;     // degrees
    double  Longitude;      // degrees
    double  Elevation;      // meters
    double  Diameter;       // meters
    double  BeamWidth;      // degrees
    double  CenterFreq;     // degrees
    double  AzOrientation;      // degrees
    double  ZenCorrCoeff[13];   // AO zenith angle corrections, arcmin.
    double  AzCorrCoeff[13];    // AO azimuth corrections, arcmin.
} ReceiverConfig_t;

void seti_cfg_GetReceiverConfig(telescope_id ReceiverID, receiver_config
        *ReceiverConfig);

void seti_cfg_GetReceiverConfig(telescope_id ReceiverID, ReceiverConfig_t *ReceiverConfig);
