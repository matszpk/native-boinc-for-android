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

#ifndef _SQLBLOB_H_
#define _SQLBLOB_H_

#ifdef swap
#undef swap
#endif

#include "sah_config.h"

#ifndef _WIN32
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#endif

#include "track_mem.h"
#include "sqldefs.h"
#include "sqlrow.h"
#include "xml_util.h"

template <typename T>
class sqlblob;

template <typename T>
std::ostream &operator <<(std::ostream &i, const sqlblob<T> &b) {
  i << xml_encode_string<T>(*(b.mem),b.encoding);
  return i;
}


template <typename T>
std::istream &operator >>(std::istream &i, sqlblob<T> &b) {
   std::string buf(""),tmp;
   std::string::size_type b_start=0,b_end=0,b_encoding=0;
   const char *enc_string;
   if (sizeof(T)==1) {
     enc_string="x-xml-entity";
   } else {
     enc_string="x-xml-values";
   }
   while (!i.eof()) {
      i >> tmp;
      buf+=(tmp+' ');
   }
   b_start=buf.find('<');
   b_end=std::min(buf.find('>',b_start+1),buf.find(' ',b_start+1));
   std::string tag(std::string("</")+std::string(buf,b_start+1,b_end-1));
   b_end=buf.find('>',b_start+1);
   b_encoding=buf.find("encoding=",b_start);
   if (b_encoding < b_end) 
     enc_string=buf.c_str()+b_encoding+strlen("encoding=");
   b_start=b_end+1;
   b_end=buf.find(tag);
   if (b.mem) {
     delete(b.mem);
#ifdef DEBUG_ALLOCATIONS
     fprintf(stderr,"sqlblob: deleted a vector at 0x%p\n",b.mem);
     fflush(stderr);
#endif
   }
   b.mem=new std::vector<T>(xml_decode_string<T>(buf.c_str()+b_start,b_end-b_start,enc_string));
   b.encoding=(xml_encoding)xml_encoding_from_string(enc_string);
#ifdef DEBUG_ALLOCATIONS
   fprintf(stderr,"sqlblob: allocated a vector at 0x%p\n",b.mem);
   fflush(stderr);
#endif
   return i;
}

       
template <typename T>
std::string xml_encode_string(const sqlblob<T> &input, 
    xml_encoding encoding=_x_xml_entity) {
  return xml_encode_string(static_cast<const T *>(&(*(input.mem))[0]),input.size(),encoding);
}


template <typename T=unsigned char>
class sqlblob : track_mem<sqlblob<T> > {
  public:
    sqlblob(const sqlblob &b);
    explicit sqlblob(const T *p=0, size_t n_elements=0, xml_encoding e=_x_hex);
    explicit sqlblob(const std::string &s,xml_encoding e=_x_hex);
    explicit sqlblob(const std::vector<T> &v, xml_encoding e=_x_hex);
    ~sqlblob();
//    operator T *() { return &((*mem)[0]); };
//    operator const T *() const { return &((*mem)[0]); };
//    operator const char *() const { return (const char *)(&((*mem)[0])); };
    T &at(int i) { return mem->at(i); }
    T &operator [](int i) { return (*mem)[i]; };
    const T operator [](int i) const { return (*mem)[i]; } ;
    sqlblob<T> &operator =(const sqlblob<T> &b);
    sqlblob<T> &operator =(const std::vector<T> &b);
    size_t size() const { return mem->size(); };
    size_t set_size(size_t n_elements);
    size_t resize(size_t n_elements);
    std::string print_xml() const;
    std::string print_hex() const;
    std::string print_raw() const;
    void parse_xml(std::string &field) { std::istringstream s(field); s >> *this; };
    typename std::vector<T>::const_iterator begin() const { return mem->begin(); };
    typename std::vector<T>::iterator begin() { return mem->begin(); };
    typename std::vector<T>::const_iterator end() const { return mem->end(); };
    typename std::vector<T>::iterator end() { return mem->end(); };
    void push_back(const T &t) { mem->push_back(t); } ;
    void clear() { mem->clear(); } ;
#if !defined(WIN32) || ( _MSC_VER > 1300 ) || defined(__MINGW32__) // VC 7 or newer
    friend std::ostream &operator << <T>(std::ostream &o, const sqlblob<T> &b);
    friend std::istream &operator >> <T>(std::istream &o, sqlblob<T> &b);
    friend std::string xml_encode_string<T>(const sqlblob<T> &b,xml_encoding encoding);
#else // VC6 or earlier
    friend std::ostream &operator <<(std::ostream &o, const sqlblob<T> &b);
    friend std::istream &operator >>(std::istream &o, sqlblob<T> &b);
    friend std::string xml_encode_string(const sqlblob<T> &b,xml_encoding encoding);
#endif
    xml_encoding encoding;
  private:
    std::vector<T> *mem;
};

template <typename T>
std::string sqlblob<T>::print_xml() const {
  return xml_encode_string<T>(*mem,encoding);
}

template <typename T>
std::string sqlblob<T>::print_hex() const {
  const char *hex="0123456789ABCDEF";
  char *p= new char [size()*sizeof(T)*2+1];
  size_t i,j;

  for (i = j = 0; i < size()*sizeof(T); i++) {
    p[j++] = hex[((*mem)[i] & 0xF0) >> 4];
    p[j++] = hex[((*mem)[i] & 0x0F)];
  }
  p[j]=0;
  std::string rv(p);
  delete p;
  return rv;
}

template <typename T>
std::string sqlblob<T>::print_raw() const {
  return std::string(reinterpret_cast<const char *>(&(*begin())),size());
}


template <typename T>
sqlblob<T>::sqlblob(const T *p, size_t len, xml_encoding e) :
  track_mem<sqlblob<T> >("sqlblob"), encoding(e) {
   if (p) {
     mem=new std::vector<T>(p,p+len);
   } else if (len) {
     mem=new std::vector<T>(len);
   } else {
     mem=new std::vector<T>();
   }
#ifdef DEBUG_ALLOCATIONS
   fprintf(stderr,"sqlblob: allocated a vector at 0x%p\n",mem);
   fflush(stderr);
#endif
}

template <typename T>
sqlblob<T>::sqlblob(const std::string &s, xml_encoding e) : 
  encoding(e), mem(new std::vector<T>(s.c_str(),s.c_str()+s.size()))
{
#ifdef DEBUG_ALLOCATIONS
   fprintf(stderr,"sqlblob: allocated a vector at 0x%p\n",mem);
   fflush(stderr);
#endif
}

template <typename T>
sqlblob<T>::sqlblob(const std::vector<T> &v, xml_encoding e) : 
     encoding(e),
     mem(new std::vector<T>(v))  {
#ifdef DEBUG_ALLOCATIONS
   fprintf(stderr,"sqlblob: allocated a vector at 0x%p\n",mem);
   fflush(stderr);
#endif
}

template <typename T>
sqlblob<T>::sqlblob(const sqlblob<T> &b) : 
     encoding(b.encoding) ,
     mem(new std::vector<T>(*(b.mem)))  {
#ifdef DEBUG_ALLOCATIONS
   fprintf(stderr,"sqlblob: allocated a vector at 0x%p\n",mem);
   fflush(stderr);
#endif
}

template <typename T>
size_t sqlblob<T>::resize(size_t len) {
  mem->resize(len);
  return mem->size();
}

template <typename T>
size_t sqlblob<T>::set_size(size_t len) {
  mem->resize(len);
  if (mem->size() < len) {
    while (mem->size() != len) {
      mem->push_back(static_cast<T>(0));
    }
  }
  return mem->size();
}

template <typename T>
sqlblob<T>::~sqlblob() {
  if (mem) {
    delete mem;
#ifdef DEBUG_ALLOCATIONS
    fprintf(stderr,"sqlblob: deleted a vector at 0x%p\n",mem);
    fflush(stderr);
#endif
  }
}

template <typename T>
sqlblob<T> &sqlblob<T>::operator =(const sqlblob<T> &b) {
  if (&b != this) {
    if (mem && (mem != b.mem)) {
      delete mem;
#ifdef DEBUG_ALLOCATIONS
      fprintf(stderr,"sqlblob: deleted a vector at 0x%p\n",mem);
      fflush(stderr);
#endif
    }
    mem=new std::vector<T>(*(b.mem));
    encoding=b.encoding;
#ifdef DEBUG_ALLOCATIONS
    fprintf(stderr,"sqlblob: allocated a vector at 0x%p\n",mem);
    fflush(stderr);
#endif
  }
  return (*this);
}

template <typename T>
sqlblob<T> &sqlblob<T>::operator =(const std::vector<T> &b) {
  if (&b != mem) {
    if (mem) {
        delete mem;
#ifdef DEBUG_ALLOCATIONS
        fprintf(stderr,"sqlblob: deleted a vector at 0x%p\n",mem);
	fflush(stderr);
#endif
    }
    mem=new std::vector<T>(b);
#ifdef DEBUG_ALLOCATIONS
    fprintf(stderr,"sqlblob: allocated a vector at 0x%p\n",mem);
    fflush(stderr);
#endif
  }
  return (*this);
}






#endif
