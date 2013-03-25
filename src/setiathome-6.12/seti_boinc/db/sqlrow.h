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

#ifndef _SQLROW_H
#define _SQLROW_H

#undef swap
#include <cstdio>
#include <string>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <sys/types.h>
#include "track_mem.h"
#include "xml_util.h"

typedef int SQL_CURSOR;

class SQL_ROW;
extern const SQL_ROW empty_row;
extern const std::string empty_string;

class SQL_ROW : track_mem<SQL_ROW> {
  public:
    explicit SQL_ROW(const std::string *s=NULL,size_t i=0);
    //    explicit SQL_ROW(const char **arr, int nfields);
    SQL_ROW(const SQL_ROW &r, size_t start_col=0, ssize_t end_col=-1);
    ~SQL_ROW();
    SQL_ROW &operator=(const SQL_ROW &r);
    std::vector<std::string *> pval;
    size_t argc(size_t i);
    size_t argc() const;
    void clear(size_t i) {
      if ((pval.size()>i) && pval[i]) {
        delete pval[i];
#ifdef DEBUG_ALLOCATIONS
        fprintf(stderr,"std::string deleted at 0x%p\n",pval[i]);
        fflush(stderr);
#endif
      }
      pval[i]=NULL;
    };
    bool exists(size_t i) const { return ((pval.size()>i) && (pval[i])); }
    void clear() { pval.resize(0); }

    //    operator char **();
    SQL_ROW operator +(size_t i) const;
    std::string *&operator [](size_t i);
    const std::string *operator [](size_t i) const;
    //    const char *operator [](int i) const;
    //    char *operator *() const;
  private:
    std::string *string_delimited(std::string &s,char c_open,char c_close);
    std::vector<std::string *> parse_delimited(std::string &s,char c_open,char c_close);
    std::vector<std::string *> parse_type(std::string &s);
    std::vector<std::string *> parse_list(std::string &s);
    std::vector<std::string *> parse_row(std::string &s);
    std::string *parse_blob(std::string &s);
};

//inline SQL_ROW::operator char **() {return static_cast<char **>(&(pval[0]));}

inline std::string *&SQL_ROW::operator [](size_t i) {return pval[i];}
inline const std::string *SQL_ROW::operator [] (size_t i) const {
  return (argc()>i)?pval[i]:&empty_string;
}

/*
inline const char *SQL_ROW::operator [](int i) const {
  if (((unsigned)i)<pval.size()) {
    return (pval[i]->c_str());
  } else {
    return "";
  }
}
*/


//inline char *SQL_ROW::operator *() const {return (pval[0]);}

inline size_t SQL_ROW::argc(size_t i) {
  if (i > pval.size()) {
    while (pval.size() != i) {
      pval.push_back(new std::string(empty_string));
#ifdef DEBUG_ALLOCATIONS
      fprintf(stderr,"std::string allocated at 0x%p\n",pval[pval.size()-1]);
      fflush(stderr);
#endif

    }
  } else {
    while (pval.size() != i) {
      if (pval[pval.size()-1]) {
#ifdef DEBUG_ALLOCATIONS
        fprintf(stderr,"std::string deleted at 0x%p\n",pval[pval.size()-1]);
        fflush(stderr);
#endif
        delete pval[pval.size()-1];
      }
      pval.pop_back();
    }
  }
  return pval.size();
}

inline size_t SQL_ROW::argc() const {return pval.size();}

inline SQL_ROW SQL_ROW::operator +(size_t i) const { return SQL_ROW(*this,i);  }

#endif
