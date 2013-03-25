//          ASMLIB.H                                        © Agner Fog 2004

// Header file for asmlibM.lib, asmlibO.lib, and asmlibE.a
// See asmlib.txt for further details

extern "C" int InstructionSet (void);         // tell which instruction set is supported
extern "C" int DetectProcessor (void);        // information about microprocessor features
extern "C" void ProcessorName (char * text);  // ASCIIZ text describing microprocessor
extern "C" int Round (double x);              // round to nearest or even
extern "C" int Truncate (double x);           // truncation towards zero
extern "C" int ReadClock (void);              // read microprocessor internal clock
extern "C" int MinI (int a, int b);           // the smallest of two integers
extern "C" int MaxI (int a, int b);           // the biggest  of two integers
extern "C" double MinD (double a, double b);  // the smallest of two double precision numbers
extern "C" double MaxD (double a, double b);  // the biggest  of two double precision numbers

