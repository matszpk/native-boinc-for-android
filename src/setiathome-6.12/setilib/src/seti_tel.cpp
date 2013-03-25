//____________________________________________________________________
// Source Code Name    : seti_tel.cc
// Programmer          : Jeff Cobb
//                     : Project SERENDIP, University of California, Berkeley CA
// Compiler            : GNU gcc
// Date Opened         : 2/99
// Purpose             : telescope IDs and routines to deal with them
// History             : 
//			3/5/99  - jeffc - added tel_GetTelTab()
//____________________________________________________________________

#include <stdio.h>
#include <string.h>
#include "setilib.h"

// ---------------------------------------------------------
unsigned char tel_GetTid(char * sReceiverID)
// ---------------------------------------------------------
// Takes a string (usually via a cmd line) and returns the
// tel ID.  Telescope ID is a field in a dbase record.
  {
  if(strcmp(sReceiverID, AO430) == 0)
    return(AO_430);
 
  if(strcmp(sReceiverID, AO1420) == 0)
    return(AO_1420);
 
  if(strcmp(sReceiverID, AOGREG1420) == 0)
    return(AOGREG_1420);
 
  return(UNKNOWN_TEL);    // unkown ID
  }


// ---------------------------------------------------------
long tel_GetTelTab(unsigned char TiD)
// ---------------------------------------------------------
// Takes tel ID and returns an index into several telescope
// specific tables.

  {
  if(TiD == AO_430)
    return(0);
  else
    if(TiD == AO_1420)
      return(1);
    else
      return(-1);
  }

