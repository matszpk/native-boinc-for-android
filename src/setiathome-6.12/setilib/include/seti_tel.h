//____________________________________________________________________
// Source Code Name    : seti_tid.h
// Programmer          : Jeff Cobb
//                     : Project SERENDIP, University of California, Berkeley CA
// Compiler            : GNU gcc
// Date Opened         : 2/99
// Purpose             : telescope IDs and routines to deal with them
// History             : 
//____________________________________________________________________


// TIDs Telescope ID numbers listed, each number is one byte
typedef enum tag_telescope_id {
  AO_430=0, AO_1420, AOGREG_1420, 
  AO_ALFA_0_0, AO_ALFA_0_1,
  AO_ALFA_1_0, AO_ALFA_1_1, 
  AO_ALFA_2_0, AO_ALFA_2_1, 
  AO_ALFA_3_0, AO_ALFA_3_1, 
  AO_ALFA_4_0, AO_ALFA_4_1, 
  AO_ALFA_5_0, AO_ALFA_5_1, 
  AO_ALFA_6_0, AO_ALFA_6_1,
  UNKNOWN_TEL=0xff
} telescope_id;
inline
telescope_id &operator++(telescope_id &tid) {
    return tid = telescope_id(tid + 1);
}

// TIDs as given on various command lines

static const char *AO430="ao430";
static const char *AO1420="ao1420";
static const char *AOGREG1420="aogreg1420";

// telescope definitions for database  (this is archaic) 
// Still needed?
#define NRAO    0x0001       /* bit 1 */
#define NRAO140 0x0002       /* bit 2 */
#define ARECIBO 0x0004       /* bit 3 */

unsigned char tel_GetTid(char * sReceiverID);
long tel_GetTelTab(unsigned char TiD);

