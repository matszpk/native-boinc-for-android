// $Id: db_table.h,v 1.45.2.10 2007/05/31 22:03:18 korpela Exp $
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

#ifndef _DB_TABLE_H_
#define _DB_TABLE_H_

#include "sah_config.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <algorithm>
#include "track_mem.h"
#include "sqlrow.h"
#include "sqlblob.h"
#include "sqlint8.h"

#ifndef CLIENT
#include "sqlapi.h"
#endif

#include "row_cache.h"

template <typename T>
class db_type : public track_mem<T> {
  private:
    static const char * const type_name;
    static const char * const column_names[];
  public:
    db_type<T>(T &t);
    ~db_type();
    db_type<T> &operator =(const T &t);
    db_type<T> &operator =(const db_type<T> &t);
    void clear() { *me = T(); };
    std::string update_format() const;
    std::string insert_format() const;
    std::string select_format() const;
    std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
    std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=1,
	const char *tag=type_name) const;
    const char *search_tag(const char *s=0);
    operator T();
    static int nfields() { return _nfields; };
  private:
    T *me;
    static const char * _search_tag;
    static const int _nfields;
};

template <typename T>
class db_table;

template <typename T>
std::ostream &operator <<(std::ostream &o, const db_type<T> &a) {
  o << a.print_xml();
  return o;
}


template <typename T>
std::ostream &operator <<(std::ostream &o, const db_table<T> &a) {
  o << a.me->print_xml();
  return o;
}

template <typename T>
std::istream &operator >>(std::istream &i, db_table<T> &a) {
  std::string s;
  static std::string remainder("");
  std::string s_tag("<");
  std::string e_tag("</");
  s_tag+=a._search_tag;
  e_tag+=a._search_tag;
  // std::string buffer(remainder);
  std::string buffer = "";
  buffer.reserve(65536);
  bool found=false, done=false, first_time = true;
  while (!i.eof()  && !done) {
    if(first_time) {
	s = remainder;
	first_time = false;
    } else
        i >> s; 
    if (!found && xml_match_tag(s,s_tag.c_str())) {
      found=true;
      a.clear();
    }
    if (found) buffer+=(s+' ');
    if (found && xml_match_tag(s,e_tag.c_str())) {
      found=false;
      done=true;
    }
  }
  a.me->parse_xml(buffer);
  std::string::size_type p=buffer.find(e_tag);
  if (p != std::string::npos) {
    p=buffer.find('>',p+1);
    if (p != std::string::npos) {
      remainder=buffer.substr(p,buffer.size()-p);
    } else {
      remainder=std::string("");
    }
  }
  return i;
}

#ifdef _WIN32
#undef DELETE
#endif

template <typename T>
class db_table : public track_mem<T> {
  public:
    db_table(T &t, SQL_CURSOR c);
    ~db_table();
#ifndef CLIENT
    sqlint8_t insert(sqlint8_t lid=0);
    std::string update_format() const;
    std::string insert_format() const;
    std::string select_format() const;
    bool update();
    bool fetch(sqlint8_t lid=0);
    bool cached_fetch(sqlint8_t lid=0);
    bool fetch(const std::string &where);
    bool authorize_delete(bool auth) {return delete_flag=auth;};
    bool DELETE(sqlint8_t lid=0);
    bool DELETE(const std::string &where);
    bool open_query(const std::string &where="",const std::string &order="",const std::string &directives="");
    bool get_next(bool cache_result);
    bool get_next() { return get_next(false); };
    bool close_query();
    static int nfields() { return _nfields; };
    sqlint8_t count(const std::string &where="");
    SQL_CURSOR get_cursor();
    int set_cache_size(int i) { cache.set_size(i); return(cache.get_size()); };
    time_t set_cache_ttl(time_t i) { cache.set_ttl(i); return(cache.get_ttl()); };
#endif
    const char *search_tag(const char *s=0);
    operator T();
    db_table<T> &operator =(const T &t);
    db_table<T> &operator =(const db_table<T> &t);
    void clear() { *me = T(); };
#if !defined(_WIN32) || (_MSC_VER > 1300)  || defined(__MINGW32__)
    friend std::ostream &operator <<<T>(std::ostream &o, const db_table<T> &a);
    friend std::istream &operator >><T>(std::istream &i, db_table<T> &a); 
#else
    friend std::ostream &operator <<(std::ostream &o, const db_table<T> &a);
    friend std::istream &operator >>(std::istream &i, db_table<T> &a); 
#endif
    static const char * const table_name;
  private:
    T *me;
    static row_cache<T> cache;
    static const char *_search_tag;
    static const int _nfields;
    static const char * const column_names[];
    bool delete_flag;
    SQL_CURSOR cursor;
};

template <typename T>
row_cache<T> db_table<T>::cache;

template <typename T, typename ID_TYPE=long>
class db_reference {
  private:
    T r;
  public:
    ID_TYPE &id;
    db_reference(ID_TYPE req_id=0);
    db_reference(const db_reference<T,ID_TYPE> &a) : r(a.r), id(*(ID_TYPE *)(&(r.id))) {};
    T &get();
    std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
    std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=1,
	const char *tag=0) const;
    void parse_xml(std::string &buf, const char *tag);
    void parse(const std::string &buf);
    void parse(const SQL_ROW &buf);
    operator T() const;
    T *operator ->();
    const T *operator ->() const;
    db_reference<T,ID_TYPE> &operator =(const T &t);
    db_reference<T,ID_TYPE> &operator =(const db_reference<T,ID_TYPE> &t);
};

template <typename T, typename ID_TYPE>
db_reference<T,ID_TYPE>::db_reference(ID_TYPE req_id) : id(*(ID_TYPE *)(&(r.id))) {
  id=req_id;
}

template <typename T>
db_type<T>::db_type(T &t) : track_mem<T>(T::type_name), me(&t) {}

template <typename T>
db_type<T>::~db_type() {}

template <typename T>
db_table<T>::db_table(T &t, SQL_CURSOR c) : track_mem<T>(T::table_name), me(&t), delete_flag(false), cursor(c) {}

template <typename T>
db_table<T>::~db_table() {}

template <typename T>
db_type<T>::operator T() {
  return *me;
}

template <typename T>
db_table<T>::operator T() {
  return *me;
}

template <typename T, typename ID_TYPE>
db_reference<T,ID_TYPE>::operator T() const {
  return r;
}

template <typename T, typename ID_TYPE>
T *db_reference<T,ID_TYPE>::operator ->() {
  return &r;
}

template <typename T, typename ID_TYPE>
const T *db_reference<T,ID_TYPE>::operator ->() const {
  return &r;
}

#ifndef CLIENT
template <typename T>
sqlint8_t db_table<T>::insert(sqlint8_t lid) {
  int rv=0;
  std::string query=std::string("insert into ")+table_name+" values ("+me->insert_format()+");";
  // jeffc
  me->id=lid;
  std::string tmpstr(me->print());
  // old SQL_ROW valarr(tmpstr.c_str(),_nfields);
  SQL_ROW valarr(&tmpstr,_nfields);
  const char *q=query.c_str();
  while (*q) {
    if (*(q++)=='?') rv++;
  }
  if ((int)valarr.argc() != rv) {
    std::cout << "Error " << valarr.argc() << "!=" << rv << std::endl;
    std::cout << query << std::endl;
    for (int i=0; i<(int)valarr.argc(); i++) {
      std::cout << i << " " << valarr[i] << std::endl;
    }
  }
#ifndef NODB
  rv=sql_open(query.c_str(),valarr);
  if (rv>=0 && sql_fetch(rv)) {
    me->id=sql_insert_id(rv);
    sql_close(rv);
    return static_cast<sqlint8_t>(me->id);
  }
#else
  std::cout << query << std::endl;
#endif
  return 0;
}

template <typename T>
sqlint8_t db_new(T &t) {
  return (static_cast<db_table<T> *>(&t)->insert());
}

template <typename T>
bool db_table<T>::update() {
  char buf[256];
  int rv,i;
  sprintf(buf," where id=%"INT8_FMT,INT8_PRINT_CAST(sqlint8_t(me->id)));
#if 0
  std::string query=std::string("update ")+table_name+" set (";
  for (i=1;i<_nfields-1;i++) query+=std::string(column_names[i])+',';
  query+=std::string(column_names[i])+")=("+me->update_format()+") ";
#else
  std::string query=std::string("update ")+table_name+" set ";
  for (i=1;i<_nfields-1;i++) query+=std::string(column_names[i])+"=?,";
  query+=std::string(column_names[i])+"=?";
#endif
  std::string tmpstr(me->print());
  // old SQL_ROW valarr(SQL_ROW(tmpstr.c_str(),_nfields)+1);
  SQL_ROW valarr(SQL_ROW(&tmpstr,_nfields)+1);
  query+=buf;
#ifndef NODB
  rv=sql_run(query.c_str(),valarr);
#else
  std::cout << query << std::endl;
  rv=true;
#endif
  return rv;
}


template <typename T>
bool db_update(T &t) {
  return (static_cast<db_table<T> *>(&t)->update());
}

template <typename T>
bool db_table<T>::DELETE(sqlint8_t lid) {
  bool rv;
  char buf[256];
  if (!delete_flag) return false;
  if (lid) me->id=lid;
  sprintf(buf," where id=%"INT8_FMT,INT8_PRINT_CAST(sqlint8_t(me->id)));
  std::string query=std::string("delete from ")+table_name+std::string(buf);
  rv=sql_run(query.c_str());
  delete_flag=false;
  return rv;
}

template <typename T>
bool db_table<T>::DELETE(const std::string &where) {
  bool rv;
  if (!delete_flag) return false;
  if (where.size() = 0) return false;
  std::string query=std::string("delete from ")+table_name+std::string(" ")+where;
  rv=sql_run(query.c_str());
  delete_flag=false;
  return rv;
}

template <typename T>
bool db_table<T>::cached_fetch(sqlint8_t lid) {
  T *item;
  if (!lid) {
    lid=me->id;
  }
  if ((item=cache.find(lid)) != NULL) {
    *this=*item;
    return true;
  } else {
    bool rv=fetch(lid);
    if (rv) cache.insert(*this);
    return rv;
  }
}

template <typename T>
bool db_table<T>::fetch(sqlint8_t lid) {
  char buf[1024];
  int tmp;
  SQL_ROW values;
  int rv;
  bool rv2=false;
  if (!lid) {
    lid=me->id;
  }
  sprintf(buf,"select * from %s where id=%"INT8_FMT,table_name,lid);
#ifndef NODB
  if (((rv=sql_open(buf))>=0) &&
      (rv2=sql_fetch(rv)))
  {
     values=sql_values(rv,&tmp); 
     me->parse(values);
  }
  if (rv>=0) sql_close(rv);
#else
  std::cout << buf << std::endl;
#endif
  return (rv2);
}

template <typename T>
bool db_table<T>::fetch(const std::string &where) {
  char buf[1024];
  int tmp;
  SQL_ROW values;
  int rv;
  bool rv2=false;
  sprintf(buf,"select * from %s %s",table_name,where.c_str());
#ifndef NODB
  if (((rv=sql_open(buf))>=0) &&
      (rv2=sql_fetch(rv)))
  {
     values=sql_values(rv,&tmp);
     me->parse(values);
  }
  if (rv>=0) sql_close(rv);
#else
  std::cout << buf << std::endl;
  rv=0;
#endif
  return (rv2);
}

template <typename T>
bool db_table<T>::open_query(const std::string &where,const std::string &order,const std::string &directives) {
  bool rv;
  std::string query=std::string("select ")+directives+" * from "+table_name+" "+where+" "+order;
#ifndef NODB
  if (cursor>=0) sql_close(cursor);
  rv=(chk_cursor(cursor=sql_open(query.c_str()),"db_table<>::open_query"));
  if (!rv) {
    sql_close(cursor);
    cursor=-1;
  }
  return (cursor>=0);
#else
  std::cout << query << std::endl;
  return true;
#endif
}

template <typename T>
bool db_table<T>::get_next(bool cache) {
  int rv;
#ifndef NODB
  if ((rv=sql_fetch(cursor))) {
    SQL_ROW r(sql_values(cursor,&rv));
    me->parse(r);
  } else {
    sql_close(cursor);
  }
  return (rv);
#else
  static int i=30;
  me->id=i;
  return ((i--)>0);
#endif
}

template <typename T>
bool db_table<T>::close_query() {
#ifndef NODB
  if (cursor>=0) sql_close(cursor);
#endif
  cursor=-1;
  return cursor;
}

template <typename T>
sqlint8_t db_table<T>::count(const std::string &where) {
  std::string query=std::string("select count(*) from ")+table_name+" "+where;
  int fd,rv=0;
  sqlint8_t lrv=-1;
#ifndef NODB
  if ((fd=sql_open(query.c_str()))>=0) {
    if (sql_fetch(fd)) {
      SQL_ROW r(sql_values(fd,&rv));
      if (rv) {
        lrv=atoll(r[0]->c_str());
      }
    } 
    sql_close(fd);
  }
  return lrv;
#else
  return 30;
#endif
}
  
#endif

template <typename T, typename ID_TYPE>
std::ostream &operator <<(std::ostream &o, const db_reference<T,ID_TYPE> &a) {
  o << a.print_xml();
  return o;
}

template <typename T>
const char *db_type<T>::search_tag(const char *s) {
  if (s) {
    _search_tag=s;
  } else {
    _search_tag=type_name;
  }
  return _search_tag;
}

template <typename T>
const char *db_table<T>::search_tag(const char *s) {
  if (s) {
    _search_tag=s;
  } else {
    _search_tag=table_name;
  }
  return _search_tag;
}

template <typename T>
std::string db_type<T>::print(int full_subtables, int show_ids, int no_refs) const {
  return me->print(full_subtables,show_ids,no_refs);
}

template <typename T>
std::string db_type<T>::print_xml(int full_subtables, int show_ids, int no_refs,
    const char *tag) const {
  return me->print_xml(full_subtables,show_ids,no_refs,tag);
}

template <typename T, typename ID_TYPE>
std::string db_reference<T,ID_TYPE>::print(int full_subtables, int show_ids, int no_refs) const {
  if (full_subtables) {
    return r.print(full_subtables,show_ids,no_refs);
  } else {
    char buf[256];
    sprintf(buf,"%"INT8_FMT,INT8_PRINT_CAST(sqlint8_t(r.id)));
    return std::string(buf);
  }
}

template <typename T, typename ID_TYPE>
std::string db_reference<T,ID_TYPE>::print_xml(int full_subtables, int show_ids, int no_refs, const char *tag) const {
  if (full_subtables) {
    return r.print_xml(full_subtables,show_ids,no_refs,tag);
  } else {
    char buf[256];
    sprintf(buf,"<id>%"INT8_FMT"</id>",INT8_PRINT_CAST(sqlint8_t(r.id)));
    return std::string(buf);
  }
}

template <typename T, typename ID_TYPE>
void db_reference<T,ID_TYPE>::parse(const std::string &buf) {
  r.parse(buf);
}

template <typename T, typename ID_TYPE>
void db_reference<T,ID_TYPE>::parse(const SQL_ROW &buf) {
  r.parse(buf);
}

template <typename T, typename ID_TYPE>
void db_reference<T,ID_TYPE>::parse_xml(std::string &buf, const char *tag) {
  r.parse_xml(buf,tag);
}

template <typename T>
db_type<T> &db_type<T>::operator =(const T &t) {
  if (me != &t) {
    *me=t;
  }
  return *this;
}

template <typename T>
db_type<T> &db_type<T>::operator =(const db_type<T> &t) {
  if (this != &t) {
    *me=*(t.me);
  }
  return *this;
}

template <typename T>
db_table<T> &db_table<T>::operator =(const T &t) {
  if (me != &t) {
    *me=t;
    cursor=-1;
  }
  return *this;
}

template <typename T>
db_table<T> &db_table<T>::operator =(const db_table<T> &t) {
  if (this != &t) {
    *me=*(t.me);
    cursor=-1;
  }
  return *this;
}

template <typename T,typename ID_TYPE>
db_reference<T,ID_TYPE> &db_reference<T,ID_TYPE>::operator =(const T &t) {
  if (&id != &(t.id)) {
    r=t;
  }
  return *this;
}

template <typename T,typename ID_TYPE>
db_reference<T,ID_TYPE> &db_reference<T,ID_TYPE>::operator =(const db_reference<T,ID_TYPE> &t) {
  if (&id != &(t.id)) {
    r=t.r;
  }
  return *this;
}


#endif
