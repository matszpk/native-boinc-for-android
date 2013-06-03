# Copyright 2003 Regents of the University of California

# SETI_BOINC is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2, or (at your option) any later
# version.

# SETI_BOINC is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.

# You should have received a copy of the GNU General Public License along
# with SETI_BOINC; see the file COPYING.  If not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

/create table/		{ n=split($3,a,"."); table=a[n]; }
/create table/,/\);/  	{ 
                          for (i=1;i<NF+1;i++) {
			    if (index($i,"references")) {
			      n=split($(i+1),a,",")
			      print table, $1, a[1]
			    } 
                          }
			}
/alter/		{ 
                  n=split($3,a,".");
		  last=$NF
		  table=a[n];
		  found=0;
                  for (i=1;i<(NF+1);i++) {
		    if (index($i,"references")) {
		      if (i==1) {
		        col=last;
			found=1;
		      } else {
		        col=$(i-1);
			found=1;
		      } 
		      if (found) {
		        if (i==NF) {
		          getline;
			  i=NF+1;
			  n=split($1,a,".");
			  rtable=a[n];
                        } else {
		          n=split($(i+1),a,".");
			  rtable=a[n];
		        }
		      }
		    } else {
		      if (i==NF) {
			getline;
			i=0;
		      }
		    }
		  }
		  if (found) print table,col,rtable;
		}
