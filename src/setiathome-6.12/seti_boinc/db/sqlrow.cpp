// Copyright 2003 Regents of the University of California

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

#include "sah_config.h"

#include <stdlib.h>
#include <string>
#include <errno.h>
#include <algorithm>
#include <vector>

#include "sqlrow.h"

const SQL_ROW empty_row;
const std::string empty_string("");

#define found(x) ((x != std::string::npos))

std::string *SQL_ROW::string_delimited(std::string &s, char c_start, char c_end)
{
  // return the substring of a string delimited by c_start and c_end.
  // c_start and c_end.  The substring returned will include everything from
  // c_start and before the comma or end or string following c_end.  
  std::string::size_type open,cls,strt,comma;
  std::string::size_type q(s.find(c_start));  // find the opening delimiter.

  if (found(q)) {
    strt=q;
    int nopen=1;
    // since any of the items inside this type could be sql defined
    // types themselves, we need to find the matching close delimiter
    do {
      open=s.find(c_start,q+1);          
      cls=s.find(c_end,q+1);
      if (found(cls) && (cls<open)) {
          nopen--;
          q=cls;
      } else if (found(open)) {
          q=open;
          nopen++;
      } else {
        // something went wrong and we didn't find a close delimiter.
	// return a null string
	std::string *p=new std::string;
#ifdef DEBUG_ALLOCATIONS
        fprintf(stderr,"std::string allocated at 0x%p\n",p);
        fflush(stderr);
#endif
	return p;
      }
    } while (nopen && (found(cls) || found(open)));
  } else {
    // there was an error.  No opening delimiter, return a null string
    std::string *p=new std::string;
#ifdef DEBUG_ALLOCATIONS
    fprintf(stderr,"std::string allocated at 0x%p\n",p);
    fflush(stderr);
#endif
    return p;
  }
  // A comma or the end of the string closes the section parsed
  comma=s.find(',',cls); 

  std::string *rv;

  if (found(comma)) {
    rv=new std::string(s,strt,comma-strt);
  } else {
    rv=new std::string(s,strt,s.size()-strt);
  }
#ifdef DEBUG_ALLOCATIONS
    fprintf(stderr,"std::string allocated at 0x%p\n",rv);
    fflush(stderr);
#endif

  // remove what we have just parsed from the head of the string
  s=s.erase(0,comma);
  return rv;
}
  

std::vector<std::string *> SQL_ROW::parse_delimited(std::string &s,char c_start,char c_end) {
  // parse a section of a comma separated string delimited by the characters 
  // c_start and c_end.  The substring parsed will include everything after
  // c_start and before the comma or end of string following c_end.  This allows
  // sql types of the format "ROW(a,b,c)::type_name" to be properly parsed 
  // to return a,b and c.
  std::string *sub(string_delimited(s,c_start,c_end));  // Get delimited string
  std::string tmp(*sub,1,sub->size()-1);  // Remove opening delimiter
  std::vector<std::string *> rv(parse_row(tmp));
  delete sub;
#ifdef DEBUG_ALLOCATIONS
  fprintf(stderr,"std::string deleted at 0x%p\n",sub);
  fflush(stderr);
#endif
  return rv;
}

std::vector<std::string *> SQL_ROW::parse_type(std::string &s) {
  // Parses informix defined types of the format "ROW(a,b,c)::typename"
  return parse_delimited(s,'(',')');
}

std::vector<std::string *> SQL_ROW::parse_list(std::string &s) {
  // Parses informix lists of the format "LIST{a,b,c}"
  return parse_delimited(s,'{','}');
}

std::string *SQL_ROW::parse_blob(std::string &s) {
  // Parsed binary objects of the format "<BYTE len=length>fasd874235vxzfdgas"
  // or binary objects consisting of the entire string.
  std::string *rv;
  std::string::size_type etag=s.find('>');
  ssize_t length;

  if (found(etag)) {
    std::string::size_type p(s.find('='));
    if (p<etag) {
      sscanf(s.c_str()+p+1,"%ld",&length);
    } else {
      length=s.size()-etag-1;
    }
  } else {
    etag=0;
    length=s.size();
  }
  rv = new std::string(s,etag+1,length);
#ifdef DEBUG_ALLOCATIONS
  fprintf(stderr,"std::string allocated at 0x%p\n",rv);
  fflush(stderr);
#endif
  s.erase(0,etag+1+length);
  return rv;
}
  

std::vector<std::string *> SQL_ROW::parse_row(std::string &s) {
  // parses a SQL row of the format "value,value,value,value", 
  // where value can be of the form:
  //   number      (i.e. 10)
  //   "string"
  //   'string'
  //   LIST{value,value,value}
  //   ROW(value,value,value)::typename
  // the passed string is consumed by this routine.
  // WARNING: NOT THREAD SAFE
  std::string::size_type comma,dquote,quote,p;
  std::vector<std::string *>::iterator i;
  std::vector<std::string *> rv;
  static bool outer=true;

  while (s.size()) {  // While there's still bits of string left to process
    // get rid of leading spaces and commas
    while (isspace(s[0]) && s.size()) s.erase(0,1);
    while (s[0]==',') s.erase(0,1);

    // Handle the special cases first...
    if (s.find("LIST")==0) {
      // only parse a LIST if the entire string passed to parse_row was a list
      if (outer) {
        outer=false;
        std::vector<std::string *> tmprow(parse_list(s));
        for (i=tmprow.begin();i<tmprow.end();i++) {
          rv.push_back(*i);
        }
      } else {
        rv.push_back(string_delimited(s,'L','}'));
      }
    } else if (s.find("ROW")==0) {
      // only parse a LIST if the entire string passed to parse_row was a ROW
      // type
      if (outer) {
        outer=false;
        std::vector<std::string *> tmprow(parse_type(s));
        for (i=tmprow.begin();i<tmprow.end();i++) {
          rv.push_back(*i);
        }
      } else {
        rv.push_back(string_delimited(s,'R',')'));
      }
    } else if (s.find("<BYTE")==0) {
      rv.push_back(parse_blob(s));
    } else {
    // now for the non-special cases...  We need to find the delimiting
    // commas for the item at the head of the line...
      comma=s.find(',');
    // but beware that the item we're parsing might be a string
      quote=s.find('\'');
      dquote=s.find('\"');
      p=std::min(std::min(comma,quote),dquote);
    // if we found none of the above, we're done.
      if (!found(p)) {
        rv.push_back(new std::string(s));
#ifdef DEBUG_ALLOCATIONS
        fprintf(stderr,"std::string allocated at 0x%p\n",rv[rv.size()-1]);
        fflush(stderr);
#endif
	s.erase();
      } else {
        switch (s[p]) {
        case ',':  // if we found a comma, this is a non quoted entry
	  rv.push_back(new std::string(s,0,p));
#ifdef DEBUG_ALLOCATIONS
          fprintf(stderr,"std::string allocated at 0x%p\n",rv[rv.size()-1]);
          fflush(stderr);
#endif
	  s.erase(0,p+1);
          break;
	case '\'': // if we found a quote we need to find the close quote 
          quote=s.find('\'',p+1);
	  if (found(quote)) {
	    // if we found it, use the quoted entity
	    rv.push_back(new std::string(s,p+1,quote-p-1));
#ifdef DEBUG_ALLOCATIONS
            fprintf(stderr,"std::string allocated at 0x%p\n",rv[rv.size()-1]);
            fflush(stderr);
#endif
	    s.erase(0,quote+2); // Assume that the character after our quote is
	                        // our comma.
	  } else {
	    // if we didn't find it, we guess that everything up to the next 
	    // comma is OK.
	    rv.push_back(new std::string(s,0,comma));
#ifdef DEBUG_ALLOCATIONS
            fprintf(stderr,"std::string allocated at 0x%p\n",rv[rv.size()-1]);
            fflush(stderr);
#endif
	    s.erase(0,comma+1);
	  }
	  break;
	case '\"': // if we found a dquote we need to find the close quote 
          dquote=s.find('\"',p+1);
	  if (found(dquote)) {
	    // if we found it, use the quoted entity
	    rv.push_back(new std::string(s,p+1,dquote-p-1));
#ifdef DEBUG_ALLOCATIONS
            fprintf(stderr,"std::string allocated at 0x%p\n",rv[rv.size()-1]);
            fflush(stderr);
#endif
	    s.erase(0,dquote+2); // Assume that the character after our quote is
	                        // our comma.
	  } else {
	    // if we didn't find it, we guess that everything up to the next 
	    // comma is OK.
	    rv.push_back(new std::string(s,0,comma));
#ifdef DEBUG_ALLOCATIONS
            fprintf(stderr,"std::string allocated at 0x%p\n",rv[rv.size()-1]);
            fflush(stderr);
#endif
	    s.erase(0,comma+1);
	  }
	  break;
	default:
	  // this should never happen
	  abort();
	}
      }
    }
    outer=false;
  }
  outer=true;
  return rv;
}


SQL_ROW::SQL_ROW(const std::string *s,size_t nfields) : track_mem<SQL_ROW>("SQL_ROW") {
  if (s) {
    std::string tmp(*s);
    pval=parse_row(tmp);
  } else {
    pval.clear();
    for (size_t i=0;i<nfields;i++) {
      pval.push_back(new std::string(empty_string));
#ifdef DEBUG_ALLOCATIONS
      fprintf(stderr,"std::string allocated at 0x%p\n",pval[i]);
      fflush(stderr);
#endif
    }
      
  }
}

/*
SQL_ROW::SQL_ROW(const char **arr, int nfields) : pval() {
  for (int i=0;i<nfields;i++) {
    std::string *p=new std::string(arr[i]);
    pval.push_back(p);
  }
}
*/

SQL_ROW::SQL_ROW(const SQL_ROW &s,size_t start_col,ssize_t end_col) : track_mem<SQL_ROW>("SQL_ROW") {
  ssize_t i;
  std::string *p;
  if (end_col<(ssize_t)start_col) end_col=(ssize_t)s.argc()-1;
  for (i=start_col;i<(end_col+1);i++) {
    if (s.exists(i)) {
      p=new std::string(*(s[i]));
#ifdef DEBUG_ALLOCATIONS
      fprintf(stderr,"std::string allocated at 0x%p\n",p);
      fflush(stderr);
#endif
    } else {
      p=0;
    }
    pval.push_back(p);
  }
}

SQL_ROW &SQL_ROW::operator =(const SQL_ROW &s) {
  if (this != &s) {
    size_t i;
    std::string *p;
    for (i=0;i<argc();i++) {
      if (pval[i]) {
        delete pval[i];
#ifdef DEBUG_ALLOCATIONS
        fprintf(stderr,"std::string deleted at 0x%p\n",p);
        fflush(stderr);
#endif
      }
    }
    pval.clear();
    for (i=0;i<s.argc();i++) {
      if (s[i]) {
        p=new std::string(*(s[i]));
#ifdef DEBUG_ALLOCATIONS
        fprintf(stderr,"std::string allocated at 0x%p\n",p);
        fflush(stderr);
#endif
      } else {
        p=0;
      }
      pval.push_back(p);
    }
  }
  return *this;
}

SQL_ROW::~SQL_ROW() {
  size_t i;
  for (i=0;i<argc();i++) {
    if (pval[i]) {
      delete pval[i];
#ifdef DEBUG_ALLOCATIONS
      fprintf(stderr,"std::string deleted at 0x%p\n",pval[i]);
      fflush(stderr);
#endif
    }
    pval[i]=0;
  }
}

