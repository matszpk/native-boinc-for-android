// SETI_BOINC is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2, or (at your option) any later
// version.

// SETI_BOINC is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.

// You should have received a copy of the GNU General Public License along
// with SETI_BOINC; see the file COPYING.  If not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

// In addition, as a special exception, the Regents of the University of
// California give permission to link the code of this program with libraries
// that provide specific optimized fast Fourier transform (FFT) functions and
// distribute a linked executable.  You must obey the GNU General Public 
// License in all respects for all of the code used other than the FFT library
// itself.  Any modification required to support these libraries must be
// distributed in source code form.  If you modify this file, you may extend 
// this exception to your version of the file, but you are not obligated to 
// do so. If you do not wish to do so, delete this exception statement from 
// your version.

// workunit_resample - a program to read in a workunit and convert it to
// real samples at twice the sampling rate with all frequencies shifted to
// be positive.

#include "sah_config.h"

#include <cstdio>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>


#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif

#include "client/timecvt.h"
#include "client/s_util.h"
#include "db/db_table.h"
#include "db/schema_master.h"
#include "seti.h"

#include "fftw3.h"


workunit_header header;

int main(int argc, char *argv[]) {
  char *outfile=NULL, buf[256];
  struct stat statbuf;
  int nbytes,nread,nsamples;
  std::string tmpbuf("");
  int i=0,j;

  if ((argc < 2) || (argc > 3)) {
    fprintf(stderr,"%s infile [outfile]\n",argv[0]);
    exit(1);
  }
  char *infile=argv[1];
  if (argc==3) {
    outfile=argv[2];
  } 
  FILE *in=fopen(infile,"r");
  FILE *out;
  if (outfile) {
    out=fopen(outfile,"w");
  } else {
    out=stdout;
  }
  stat(infile,&statbuf);
  nbytes=statbuf.st_size;
  fseek(in,0,SEEK_SET);
  tmpbuf.reserve(nbytes);
  // read entire file into a buffer.
  while ((nread=(int)fread(buf,1,sizeof(buf),in))) {
    tmpbuf+=std::string(&(buf[0]),nread);
  }
  // parse the header
  header.parse_xml(tmpbuf);
  // decode the data
  std::vector<unsigned char> datav(
    xml_decode_field<unsigned char>(tmpbuf,"data") 
  );
  tmpbuf.clear();
  nsamples=header.group_info->data_desc.nsamples;
  nbytes=nsamples*header.group_info->recorder_cfg->bits_per_sample/8;
  if (datav.size() < nbytes) {
    fprintf(stderr,"Data size does not match number of samples\n");
    exit(1);
  }
  // convert the data to floating point
  sah_complex *fpdata=(sah_complex *)calloc(nsamples,sizeof(sah_complex));
  sah_complex *fpout=(sah_complex *)calloc(2048,sizeof(sah_complex));
  sah_complex *tmpb=(sah_complex *)calloc(1024,sizeof(sah_complex));
  if (!fpdata || !fpout) {
    fprintf(stderr,"Memory allocation failure\n");
    exit(1);
  }
  bits_to_floats(&(datav[0]),fpdata,nsamples);
  datav.clear();

  sah_complex workbuf[2048];
  fftwf_plan reverse=fftwf_plan_dft_1d(2048,workbuf,fpout,FFTW_BACKWARD,FFTW_MEASURE);
  fftwf_plan forward=fftwf_plan_dft_1d(1024,tmpb,workbuf,FFTW_FORWARD,FFTW_MEASURE|FFTW_PRESERVE_INPUT);

  while (i<nsamples) {
    // Do the forward transform.
    fftwf_execute_dft(forward, fpdata+i, workbuf);
    // shift 64 frequency bins
    memmove((void *)(workbuf+64), (void *)workbuf, 1024*sizeof(sah_complex));
    // now move the upper 64 into the low bins
    memmove((void *)workbuf,(void *)(workbuf+1024),64*sizeof(sah_complex));
    // clear the upper range
    memset((void *)(workbuf+1024),0,64*sizeof(sah_complex));
    // Do the reverse transform
    fftwf_execute_dft(reverse,workbuf,fpout);
    //
    for (j=0; j<2048; j++) {
      fprintf(out,"%f\n",fpout[j][0]/1024.0);
    }
    i+=1024;
  }
  exit(0);
}


