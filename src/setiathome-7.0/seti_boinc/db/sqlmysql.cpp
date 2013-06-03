// Copyright (c) 1999-2013 Regents of the University of California
//
// FFTW: Copyright (c) 2003,2006-2011 Matteo Frigo
//       Copyright (c) 2003,2006-2011 Massachusets Institute of Technology
//
// fft8g.[cpp,h]: Copyright (c) 1995-2001 Takya Ooura
//
// ASMLIB: Copyright (c) 2004 Agner Fog

// This program is free software; you can redistribute it and/or modify it 
// under the terms of the GNU General Public License as published by the 
// Free Software Foundation; either version 2, or (at your option) any later
// version.

// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.

// You should have received a copy of the GNU General Public License along
// with this program; see the file COPYING.  If not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

// In addition, as a special exception, the Regents of the University of
// California give permission to link the code of this program with libraries
// that provide specific optimized fast Fourier transform (FFT) functions
// as an alternative to FFTW and distribute a linked executable and 
// source code.  You must obey the GNU General Public License in all 
// respects for all of the code used other than the FFT library itself.  
// Any modification required to support these libraries must be distributed 
// under the terms of this license.  If you modify this program, you may extend 
// this exception to your version of the program, but you are not obligated to 
// do so. If you do not wish to do so, delete this exception statement from 
// your version.  Please be aware that FFTW and ASMLIB are not covered by 
// this exception, therefore you may not use FFTW and ASMLIB in any derivative 
// work so modified without permission of the authors of those packages.

#include "sah_config.h"

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cctype>
#include <unistd.h>

#include "sqlapi.h"

struct CURSOR {
  CURSOR() : active(0), is_select(1), rp(0), current_row(0) {};
  int active;
  int is_select;
  MYSQL_RES *rp;
  MYSQL_ROW current_row;
};

MYSQL* mysql;
char sql_dbname[256];

const char *myerrormsg;

CURSOR cursor_arr[sql::MAXCURSORS];

bool chk_cursor(SQL_CURSOR fd, const char *s) {
  bool rv=true; 
  if (fd<0) rv=false;
  if (fd>(sql::MAXCURSORS-1)) rv=false;
  if (rv) rv=(cursor_arr[fd].active);
  if (!rv) {
    myerrormsg=s;
    fprintf(stderr,"%s: %s\r\n",s,sql_error_message());
  } else {
    myerrormsg=0;
  }
  return (rv);
}





// convert ' to \' in place
static void escape_single_quotes(char* field) {
  char buf[sql::MAX_QUERY_LEN];
  char* q = buf, *p = field;
  while (*p) {
    if (*p == '\'') {
      *q++ = '\\';
      *q++ = '\'';
    } else {
      *q++ = *p;
    }
    p++;
  }
  *q = 0;
  strcpy(field, buf);
}

static void unescape_single_quotes(char* p) {
  char* q;
  while (1) {
    q = strstr(p, "\\'");
    if (!q) break;
    strcpy(q, q+1);
  }
}

bool sql_database(const char* dbname, const char* password, int exclusive) {
  char buf[256], *p, *host;
  if ((!dbname) || (strlen(dbname)==0)) {
    dbname=getenv("DATABASE");
  }
  buf[0]=0;
  if (dbname) {
    strncpy(buf,dbname,254);
    strncpy(sql_dbname,dbname,254);
    buf[255]=0;
  }
  if ((p=strchr(buf,'@'))) {
    host=p+1;
    *p=0;
  } else {
    host=0;
  }
  mysql = mysql_init(0);
  if (!mysql) {
    fprintf(stderr,"mysql_init() failed:");
    fprintf(stderr,"%s\r\n",sql_error_message());
    return false;
  }
  
  if (buf[0]) {
    mysql = mysql_real_connect(mysql, host, 0, password, buf, 0, 0, 0);
  } else {
    mysql = mysql_real_connect(mysql, host, 0, password, 0, 0, 0, 0);
  }

  if (!mysql) {
    fprintf(stderr,"mysql_real_connect(%p,%s,0,%s,%s,0,0,0) failed:",mysql,host,password,buf);
    fprintf(stderr,"%s\r\n",sql_error_message());
    return false;
  }
  return true;
}

bool sql_run(const char *stmt, SQL_ROW &argv) {
  SQL_CURSOR fd;
  bool ret=true;

  fd = sql_open(stmt, argv);
  if (ret=chk_cursor(fd, stmt)) {
    if (cursor_arr[fd].is_select) {
      ret=sql_fetch(fd);
      if (ret) {
	argv=sql_values(fd);
      } else {	
        ret=chk_cursor(fd, stmt);
      }
    }  
  }
  if (fd>=0) sql_close(fd);
  return ret;
}

int begin_work() {
  return mysql_query(mysql,"begin");
}

int commit_work() {
  return mysql_query(mysql,"commit");
}

int rollback_work() {
  return mysql_query(mysql,"rollback");
}

bool sql_finish() {
  mysql_close(mysql);
  return 1;
}

sqlint8_t sql_insert_id(SQL_CURSOR fd) {
  return mysql_insert_id(mysql);
}

int sql_error_code() {
  return mysql_errno(mysql);
}

const char *sql_error_message() {
  return mysql?mysql_error(mysql):"Not connected";
}

bool sql_exists(const char *table, const char *field, const char *value,
                const char *where) {
  char stmt[sql::MAX_QUERY_LEN];
  SQL_CURSOR c;
  int rv=false;
  if (value && *value) {
    sprintf(stmt,"select '1' from %s where %s = %s", table, field, value);
  } else {
    sprintf(stmt,"select '1' from %s",table);
    if (where && *where) {
      sprintf(stmt+strlen(stmt),"where %s",where);
    }
  }
  if (((c=sql_open(stmt))>=0) && sql_fetch(c)) {
    rv=true;
  }
  sql_close(c);
  return rv;
}

bool sql_fetch(SQL_CURSOR c) {
  return ((!(cursor_arr[c].is_select)) 
      || (cursor_arr[c].active && cursor_arr[c].rp 
          && (cursor_arr[c].current_row=mysql_fetch_row(cursor_arr[c].rp)))) ;
}

SQL_ROW sql_values(SQL_CURSOR c, int *num, int dostrip) {
  SQL_ROW rv;
  if (cursor_arr[c].active && cursor_arr[c].rp) {
    MYSQL_RES *rp=cursor_arr[c].rp;
    const char **row=const_cast<const char **>(cursor_arr[c].current_row);
    if (row) {
      if (num) { 
        *num=mysql_num_fields(rp);
      }
      rv=SQL_ROW(row,mysql_num_fields(rp));
    } 
  }
  return rv;
}

SQL_CURSOR allocate_cursor() {
  int i=0;
  while (cursor_arr[i].active && (i<sql::MAXCURSORS)) i++;
  if (i<sql::MAXCURSORS) {
    cursor_arr[i].active=true;
  } else {
    fprintf(stderr,"Unable to allocate cursor\r\n",i);
    i=-1;
  }
  return i;
}

bool free_cursor(SQL_CURSOR c) {
  cursor_arr[c].active=false;
  return !cursor_arr[c].active;
}

bool store_result(SQL_CURSOR c) {
  cursor_arr[c].rp=mysql_store_result(mysql);
  return cursor_arr[c].rp;
}

bool free_result(SQL_CURSOR c) {
  if (cursor_arr[c].rp) {
    mysql_free_result(cursor_arr[c].rp);
    cursor_arr[c].rp=0;
  }
  return cursor_arr[c].active;
}

bool sql_close(SQL_CURSOR c) {
  free_result(c);
  return free_cursor(c);
}



int sql_open(const char *stmt, SQL_ROW &argv) {
  const char *p=stmt;
  int n_sub=0,rv=-1;
  SQL_CURSOR c=-1;
  std::string query("");

  while (*p != 0) {
    if (*p=='?') {
      if (n_sub<argv.argc()) {
        query+=argv[n_sub];
      }
      n_sub++;
    } else {
      query+=*p;
    }
    p++;
  }
  //printf("argv.argc()= %d",argv.argc()); fflush(stdout);
  if ((argv.argc()==n_sub) && ((c=allocate_cursor())>=0)) {
    //printf("mysql= %p",mysql); fflush(stdout);
    rv=mysql_real_query(mysql,query.c_str(),query.size());
    //printf("rv= %d",rv); fflush(stdout);
  } else {
    //fprintf(stderr,"field mismatch: %d != %d in %s\r\n",argv.argc(),n_sub,query.c_str());
  }
  if (rv) {
    chk_cursor(c,"mysql_real_query");
    free_cursor(c);
    return -1;
  }
  if (store_result(c)) {
    cursor_arr[c].is_select=1;
  } else {
    if (mysql_field_count(mysql)!=0) {
      fprintf(stderr,"mysql_field_count()!=0 : %s\r\n",query.c_str());
      free_cursor(c);
      return -1;
    }
    cursor_arr[c].is_select=0;
  }
  return c;
}

