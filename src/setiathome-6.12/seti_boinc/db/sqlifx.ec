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
#undef USE_MYSQL
#define loc_t sys_loc_t
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <algorithm>
#include <vector>
#undef loc_t

#include "sqlint8.h"
#include "sqlca.h"
#include "sqlda.h"
#include "sqlhdr.h"
#include "locator.h"
#include "sqltypes.h"
#include "sqlstype.h"
#include "sqldefs.h"
#include "sqlint8.h"
#include "sqlblob.h"
#include "sqlrow.h"
#include "sqlapi.h"


extern sqlca_s sqlca;

#ifdef USE_NAMESPACE
namespace ifx {
#endif

#define found(x) ((x != std::string::npos))

struct IFX_LOC_T : private track_mem<IFX_LOC_T>, public loc_t {
  IFX_LOC_T() : track_mem<IFX_LOC_T>("IFX_LOC_T") { 
//    fprintf(stderr,"Creating IFX_LOC_T, this=%x\n",this);
//    fflush(stderr);
    memset(this,0,sizeof(IFX_LOC_T)); 
  };  
  IFX_LOC_T(const IFX_LOC_T &s) : track_mem<IFX_LOC_T>("IFX_LOC_T") {
//    fprintf(stderr,"Creating IFX_LOC_T, this=%x\n",this);
//    fflush(stderr);
    memcpy(this,&s,sizeof(IFX_LOC_T));
    if (s.loc_buffer) {
      if (s.loc_bufsize) {
        loc_buffer=new char [s.loc_bufsize];
      } else {
        loc_buffer=new char [strlen(s.loc_buffer)];
	loc_bufsize=strlen(s.loc_buffer)+1;
	loc_size=loc_bufsize;
      }
      memcpy(loc_buffer,s.loc_buffer,loc_bufsize);
    }
  } 
  IFX_LOC_T(const std::string &s) : track_mem<IFX_LOC_T>("IFX_LOC_T") {
//    fprintf(stderr,"Creating IFX_LOC_T, this=%x\n",this);
//    fflush(stderr);
    memset(this,0,sizeof(IFX_LOC_T));
    loc_loctype=LOCMEMORY;
    loc_buffer=new char [s.size()+1];
    loc_bufsize=s.size();
    loc_size=loc_bufsize;
    memcpy(loc_buffer,s.c_str(),s.size());
    loc_buffer[s.size()]=0;
  }

  IFX_LOC_T(const char *s, size_t l) : track_mem<IFX_LOC_T>("IFX_LOC_T") {
//    fprintf(stderr,"Creating IFX_LOC_T, this=%x\n",this);
//    fflush(stderr);
    memset(this,0,sizeof(IFX_LOC_T));
    loc_loctype=LOCMEMORY;
    loc_buffer=new char [l+1];
    loc_bufsize=l;
    loc_size=loc_bufsize;
    memcpy(loc_buffer,s,l);
    loc_buffer[l]=0;
  }

  ~IFX_LOC_T() {
//    fprintf(stderr,"In ~IFX_LOC_T()\n");
//    fflush(stderr);
    if (loc_buffer) delete [] loc_buffer;
    loc_buffer=NULL;
  }
};

struct SQLVAR : private track_mem<SQLVAR>, public ifx_sqlvar_t {
  static int ref_cnt;
  void clear();
  bool is_blob() { 
    return ((sqltype == CLOCATORTYPE) ||
             ((sqltype & SQLTYPE) == SQLTEXT) ||
	     ((sqltype & SQLTYPE) == SQLBYTES)); 
  } 
  SQLVAR &operator =(const SQLVAR &s);
  SQLVAR();
  SQLVAR(const SQLVAR &s);
  SQLVAR(const std::string &s);
  SQLVAR(const IFX_LOC_T &s);
  ~SQLVAR();
};

int SQLVAR::ref_cnt;
SQLVAR::SQLVAR() {
//  fprintf(stderr,"Creating SQLVAR, this=%x\n",this);
//  fflush(stderr);
  sqldata=0;
  sqltype=0;
  ref_cnt++;
}

SQLVAR::SQLVAR(const SQLVAR &s) : track_mem<SQLVAR>("SQLVAR") {
//  fprintf(stderr,"Creating SQLVAR, this=%x\n",this);
//  fflush(stderr);
  ref_cnt++;
  memcpy(this,&s,sizeof(SQLVAR));
  if (sqltype == CLOCATORTYPE) {
    IFX_LOC_T *sloc=(IFX_LOC_T *)s.sqldata;
    IFX_LOC_T *loc=new IFX_LOC_T(*sloc);
    sqldata=(char *)loc;
  } else {
    sqldata=new char [s.sqllen+1]; 
    memcpy(sqldata,s.sqldata,s.sqllen);
    sqldata[s.sqllen]=0;
  }
}

SQLVAR::SQLVAR(const std::string &s)  : track_mem<SQLVAR>("SQLVAR") {
//    fprintf(stderr,"Creating SQLVAR, this=%x\n",this);
//    fflush(stderr);
    ref_cnt++;
    memset(this,0,sizeof(SQLVAR));
    sqltype=CCHARTYPE;
    sqllen=s.size();
    sqldata=new char [s.size()+1]; 
    memcpy(sqldata,s.c_str(),s.size()+1);
    sqldata[s.size()]=0;
}

SQLVAR::SQLVAR(const IFX_LOC_T &s) : track_mem<SQLVAR>("SQLVAR") {
//    fprintf(stderr,"Creating SQLVAR, this=%x\n",this);
//    fflush(stderr);
    ref_cnt++;
    memset(this,0,sizeof(SQLVAR));
    sqltype=CLOCATORTYPE;
    IFX_LOC_T *loc=new IFX_LOC_T(s);
    sqllen=sizeof(IFX_LOC_T);
    sqldata=(char *)loc;
}

SQLVAR &SQLVAR::operator =(const SQLVAR &s) {
  if (this != &s) {
    clear();
    memcpy(this,&s,sizeof(SQLVAR));
    if (sqltype == CLOCATORTYPE) {
      IFX_LOC_T *sloc=(IFX_LOC_T *)s.sqldata;
      IFX_LOC_T *loc=new IFX_LOC_T(*sloc);
      sqldata=(char *)loc;
    } else {
      sqldata=new char [s.sqllen+1]; 
      memcpy(sqldata,s.sqldata,s.sqllen);
      sqldata[s.sqllen]=0;
    }
  }
  return *this;
}
    


void SQLVAR::clear() {
  if (sqldata) {
    if (sqltype == CLOCATORTYPE) {
      delete (IFX_LOC_T *)sqldata;
    } else {
      delete [] sqldata;
    }
    sqldata=NULL;
  }
}

SQLVAR::~SQLVAR() {
  ref_cnt--;
//  fprintf(stderr,"Destroying SQLVAR() this=%x\n",this);
//  fflush(stderr);
  clear();
}

struct SQLDA : public track_mem<SQLDA>, public ifx_sqlda_t {
  void resize(int argc) {
    int i;
    if (argc>sqld) {
      SQLVAR *tmp=(SQLVAR *)sqlvar;
      sqlvar=new SQLVAR [argc];
      if (tmp) {
        for (i=0;i<sqld;i++) {
          sqlvar[i]=tmp[i];
        }
        delete [] tmp;
      }
    }
    if ((!argc) && sqlvar) delete [] (SQLVAR *)sqlvar;
    sqld=argc;
  }
  SQLDA(int argc=0) {
//    fprintf(stderr,"Creating SQLDA, this=%x\n",this);
//    fflush(stderr);
    sqld=0;
    desc_name[0]=0;
    desc_occ=sizeof(SQLDA);
    desc_next=0;
    reserved=0;
    sqlvar=0;
    if (argc) resize(argc);
  }
  ~SQLDA() { 
//    fprintf(stderr,"Destroying SQLDA, this=%x\n",this);
//    fflush(stderr);
    delete [] (SQLVAR *)sqlvar;
    sqlvar=NULL;
  };
};

struct SSQL : private track_mem<SSQL> {
  char       *cmd;
  unsigned int opened:1;
  unsigned int is_select:1;
  SQLDA *in;
  SQLDA *out;
  SQLDA *exec;
  int *arr_sqltype;
  int arr_sqltype_len;
  int *arr_sqllen;
  char          p_name[19];
  char          c_name[19];
  sqlint8_t insert_id;
  SSQL() : cmd(0), opened(0), is_select(0), in(0), out(0), exec(0),
           arr_sqltype(0), arr_sqltype_len(0), arr_sqllen(0), insert_id(0) {
    p_name[0]=0;
    c_name[0]=0;
  }
};

static char *null_cmd="";

#define debug(s)    s

#define errmalloc(pointer, func, len)   \
    if (!pointer) {\
        sprintf(errmsg, "Cannot malloc for %uld in %s errno %d",len,func,errno);\
        debug(fprintf(stderr, "%s\n", errmsg));\
        return false;\
    }

#define is_hex_digit(c) \
    ((((c)>='0') && ((c)<='9')) || (((c)>='a') && ((c)<='f')) || (((c)>='A') && ((c)<='F')))
 

#define make_into_char(arg) \
    (((arg) >= 'A' && (arg) <= 'F')?((arg) - 'A' + 10):\
    (((arg) >= 'a' && (arg) <= 'f')?((arg) - 'a' + 10):(arg-'0')))

bool print_status(const char *p1,const char *p2);


int chk_status(int fd, const char *p1, const char *p2) {
  int sav=sql_error_code();
  if (sav != 0) {
    if (sav < 0) {
      sql_close(fd);
      print_status(p1, p2);
      return (sav);
    }
    print_status(p1,p2);
  }
  return 0;
}


static SSQL ssql[sql::MAXCURSORS+1];
static char *database_name;
static char *connection_name;

static char errmsg[1024+1];

#ifndef lint
static char const rcs[] = "@(#)$Id: sqlifx.ec,v 1.22.2.7 2007/04/23 22:04:07 jeffc Exp $";
#endif

sqlint8_t sql_insert_id() {
  sqlint8_t rv(0);
  ifx_int8_t val;
  char a[256];
  a[0]=0;
  ifx_getserial8(&val);
  ifx_int8toasc(&val,&a[0],24);
  a[255]=0;
  sscanf(a,"%"INT8_FMT,&rv);
  if (!rv) {
    rv=static_cast<sqlint8_t>(sqlca.sqlerrd[1]);
  }
  return rv;
}

#define INSERT_ID sql_insert_id()

sqlint8_t sql_insert_id(SQL_CURSOR fd) {
  return ssql[fd].insert_id;
}


bool chk_cursor(SQL_CURSOR fd, const char *function) {
  if (fd < 0 || fd >= sql::MAXCURSORS || !ssql[fd].cmd) {
    sprintf(errmsg, "Invalid fd %d passed to %s()", fd, function);
    debug(fprintf(stderr, "%s\n", errmsg));
    return false;
  }
  return true;
}

/* removes trailing white spaces */
static void strip(std::string &s) {
  while (s.size() && isspace(s[s.size()-1])) {
    s.erase(s.size()-1,1);
  }
}

static void strip(char *s, int len) {
  while (len>=0 && s[len]) s[len--]=0;
}

static void undelimit(std::string &s,char c_start,char c_end) {
  while (s.size() && (isspace(s[0]) || (s[0]==c_start))) s.erase(0,1);
  while (s.size() && (isspace(s[s.size()-1]) || (s[s.size()-1]==c_end))) s.erase(s.size()-1,1);
}

/* Using the previous SQL error code, format the Informix error message */
bool print_status(const char *p1, const char *p2) {
  int         retcode = sql_error_code();
  if (retcode < 0) {
    int msglen;
    errmsg[0] = '\0';
    rgetlmsg(sql_error_code(), errmsg, sizeof(errmsg), &msglen);
    if (sqlca.sqlerrd[1]) {
      int len = strlen(errmsg);
      errmsg[len] = '\0';
      rgetlmsg(sqlca.sqlerrd[1], &errmsg[len], sizeof(errmsg) - len, &msglen);
    }
    strip(sqlca.sqlerrp,strlen(sqlca.sqlerrp));
    sprintf(&errmsg[strlen(errmsg)],
            "%d:%ld:%s %s %s %s\n",
            sql_error_code(), sqlca.sqlerrd[1], sqlca.sqlerrp, sqlca.sqlerrm, p1, p2);
    debug(fprintf(stderr, "%s\n", errmsg));
  }
  return (retcode>=0);
}

/* Set explain on or off */
bool sql_explain(int n) {
  int fd=0;
  if (n) {
    $set explain on;
    chk_status(fd,"sql_explain","set explain on");
  } else {
    $set explain off;
    chk_status(fd,"sql_explain","set explain off");
  }
  return ((sql_error_code())==0);
}

/* Return the error message stored in the buffer */
const char *sql_error_message(void) {
  return errmsg;
}

/*
** Check whether column (field) exists in table with value specified by val
** subject to additional constraints specified in where.
**
** returns
**  true if field exists in table,
**  false otherwise
*/
static int alloc_stmt();

bool sql_exists(const char *table, const char *field, const char *p_val, const char *where) {
  $char *value = const_cast<char *>(p_val);
  $char stmt[sql::MAXBLOB+1];
  $char result[2];
  int fetch_code = 0; /* NOTUSED */
  int fd=alloc_stmt();

  if (value && *value != '\0') {
    sprintf(stmt, "select '1' from %s where %s = ?",
            table, field);
    if (where && *where != '\0')
      sprintf(&stmt[strlen(stmt)], " and %s", where);
  } else {
    sprintf(stmt, "select '1' from %s",
            table);
    if (where && *where != '\0')
      sprintf(&stmt[strlen(stmt)], " where %s", where);
  }

  $prepare pexist from $stmt;
  if (chk_status(fd,"PREPARE(EXISTS)", stmt)) return false;
  $declare pexist_cur cursor for pexist;
if (chk_status(fd,"DECLARE(EXISTS)", stmt)) return false;
  if (value && *value != '\0')
    $open pexist_cur using $value;
  else
    $open pexist_cur;
  if (chk_status(fd,"OPEN(EXISTS)", stmt)) return false;
  $fetch pexist_cur into $result;
  fetch_code = sql_error_code();
  $close pexist_cur;
  sql_close(fd);
  return (fetch_code==0);
}



std::string decode_hex(std::string input) {
  std::string tgt;
  int i,j;

  tgt.resize(input.size()/2);

  for (i = j = 0; i < input.size(); i += 2) {
    if (!is_hex_digit(input[i]) || !is_hex_digit(input[i+1])) return input;
    tgt[j++] = make_into_char(input[i])*16 + make_into_char(input[i+1]);
  }
  return tgt;
}

static bool sql_set_values(SQL_CURSOR fd, SQL_ROW &argv) {
  SQLVAR *col;
  SQLVAR *ecol;
  SQLDA *udesc;
  SQLDA *edesc;
  int i;
  int argc=argv.argc();

  udesc = ssql[fd].in;
  edesc = ssql[fd].exec;
  if (argc > 0) {
    if (edesc && (argc > edesc->sqld)) {
      edesc->resize(argc);
    }
    if (udesc && argc != udesc->sqld) {
      sprintf(errmsg,
              "Incorrect number (%d) of bind variables specified - %d required",
              argc, udesc->sqld);
      debug(fprintf(stderr, "%s\n", errmsg));
      return false;
    }
    if (udesc) delete (SQLDA *)udesc; 
  
    udesc = ssql[fd].in = new SQLDA(argc);

    if (edesc && edesc->sqld <= 0) edesc = NULL;
    if (edesc && edesc->sqlvar) ecol = (SQLVAR *)edesc->sqlvar;
    else ecol = NULL;

    for (i = 0, col = (SQLVAR *)udesc->sqlvar; i < argc; i++, col++) {
      int blob = 0;
      /* Is our column a blob? */
      if (ecol) {
        if (ecol->is_blob()) blob = 1;
      }

      if (!argv.exists(i)) argv[i]=new std::string("");
      /* remove leading and trailing single quotes */
      undelimit(*(argv[i]),'\'','\'');

      /* Check for blobs */
      if (blob) {
        /* Remove tags */
        if ((argv[i]->find("<BYTE") == 0) || (argv[i]->find("<TEXT")==0)) {
	  std::string::size_type etag=argv[i]->find('>');
	  if (found(etag)) argv[i]->erase(0,etag+1);
	}
        std::string tmpstr=decode_hex(*(argv[i]));
	IFX_LOC_T tmploc=IFX_LOC_T(tmpstr);
	SQLVAR tmpvar=SQLVAR(tmploc);
        *col=tmpvar;
      } else  {
	SQLVAR tmpvar=SQLVAR(*(argv[i]));
        *col=tmpvar;
      }
      col->sqlname = 0;
      col->sqlind = 0;
      if (ecol) ecol++;
    }
  }
  return true;
}

/* Returns
    true on successful execution
    sqlca.sqlcode otherwise
*/
bool sql_run(const char *stmt, SQL_ROW &argv) {
  SQL_CURSOR fd;
  int ret;

  if (isdigit(stmt[0])) {
    fd = atoi(stmt);
    if (!chk_cursor(fd, "sql_run"))
      sql_close(fd);
    return false;
    if (!sql_set_values(fd, argv))
      sql_close(fd);
    return false;
    return sql_fetch(fd);
  }
  fd = sql_open(stmt, argv);
  if (fd < 0) {
    sql_close(fd);
    return false;
  }
  ret = sql_fetch(fd);
  sql_close(fd);
  return ret;
}

/*
** Allocate a statement number
** Returns:
**   -1: no available statements
**   0..sql::MAXCURSORS-1: new statement number
*/
static int alloc_stmt() {
  int i;

  for (i = 0; i < sql::MAXCURSORS; i++) {
    if (!ssql[i].cmd)
      break;
  }
  if (i >= sql::MAXCURSORS) {
    sprintf(errmsg,
            "FATAL ERROR: Too many open SQL statements (> %d) for sql operations\n", sql::MAXCURSORS);
    debug(fprintf(stderr, "%s\n", errmsg));
    i = -1;
    exit(1);
  } else {
    ssql[i].cmd=null_cmd;
    sprintf(ssql[i].p_name, "p_%d", i);
    sprintf(ssql[i].c_name, "c_%d", i);
  }
  return(i);
}

SQL_CURSOR sql_open(const char *sql, SQL_ROW &argv) {
  SQLVAR *col = 0L;
  ifx_sqlda_t *udesc = 0L;
  $char *stmt = const_cast<char *>(sql);
  int         i;
  int         fd;
  int         len;
  char       *save;
  $char *p_name;
  $char *c_name;
  int rv;

  if ((fd = alloc_stmt()) < 0)
    return(-1);

  len = strlen(stmt);
  save = new char [len+1];
  strcpy(save, stmt);
  ssql[fd].cmd = save;

  p_name = ssql[fd].p_name;

  $prepare $p_name from $stmt;

  if (rv=chk_status(fd,"PREPARE(OPEN)", stmt)) return rv;

  $describe $p_name into udesc;

  if (rv=chk_status(fd,"DESCRIBE(OPEN)", stmt)) return rv;

  /**
  ** The cursory statements are:
  ** -- EXECUTE PROCEDURE
  ** -- SELECT without INTO TEMP
  ** Fortunately, SELECT INTO TEMP is given code SQ_SELINTO
  ** Unfortunately, SELECT is not given code SQ_SELECT (2) but 0
  ** Non-cursory statements need minimal extra attention.
  */
  if ((sql_error_code()) != 0 &&
      ((sql_error_code()) != SQ_EXECPROC || udesc->sqld == 0) &&
      (sql_error_code()) != SQ_SELECT) {
    ssql[fd].is_select = 0;
    ssql[fd].exec = (SQLDA *)udesc;
    if (!sql_set_values(fd, argv))
      return sql_close(fd), -1;
    return fd;
  }

  ssql[fd].is_select = 1;
  ssql[fd].out = (SQLDA *)udesc;

  if (!udesc) {
    sprintf(errmsg, "No columns selected !\n");
    fprintf(stderr, "No columns selected !\n");
    return -1;
  }

  for (i = 0, col = (SQLVAR *)udesc->sqlvar; i < udesc->sqld; i++, col++) {
    int         blob = 0;
    if ((col->sqltype & SQLTYPE) != SQLCHAR &&
        (col->sqltype & SQLTYPE) != SQLVCHAR) {
      if (col->is_blob()) {
        blob = 1;
      } else if ((col->sqltype & SQLTYPE) == SQLROW) {
        col->sqllen = sql::MAXBLOB;
      } else if ((col->sqltype & SQLTYPE) == SQLLIST) {
        col->sqllen = sql::MAXBLOB;
      } else {
        col->sqllen = rtypwidth(col->sqltype, col->sqllen);
      }
      /* Should allocate at least 12 characters for backward compat. ! */
      if (col->sqllen < 12) col->sqllen = 12;
    }
    if (blob) {
      IFX_LOC_T tmploc;
      tmploc.loc_loctype=LOCMEMORY;
      tmploc.loc_bufsize=-1;
      SQLVAR tmpvar(tmploc);
      *col=tmpvar;
    } else {
      std::string tmpstr(col->sqllen+2,0);
      col->sqllen++;
      SQLVAR tmpvar(tmpstr);
      *col=tmpvar;
    }
    col->sqlind = new short;
    *(col->sqlind) = 0;
  }

  c_name = ssql[fd].c_name;

  $declare $c_name cursor for $p_name;

  if (rv=chk_status(fd,"DECLARE(OPEN)", stmt)) return rv;

  if (!sql_reopen(fd,argv)) {
    sql_close(fd);
    fd=-1;
  }
  if (rv=chk_status(fd,"OPEN(OPEN)", stmt)) return rv;

  ssql[fd].opened = 1;
  ssql[fd].insert_id=INSERT_ID;
  return fd;
}

static int LAST_NON_ZERO_SQLCODE;

int sql_error_code() {
  if (SQLCODE != 0) LAST_NON_ZERO_SQLCODE = SQLCODE;
  return SQLCODE;
}

int sql_last_error_code() {
  return LAST_NON_ZERO_SQLCODE;
}

void sql_reset_error_code() {
  LAST_NON_ZERO_SQLCODE=0;
}


bool sql_reopen(SQL_CURSOR fd, SQL_ROW &argv) {
  struct sqlda *udesc;
  $ char *c_name;

  if (!chk_cursor(fd, "sql_reopen"))
    return false;

  if (!sql_set_values(fd, argv))
    return false;

  udesc = ssql[fd].in;
  c_name = ssql[fd].c_name;

  if (udesc && udesc->sqld)
    $open $c_name using descriptor udesc;
  else
    $open $c_name;

  if (chk_status(fd,"OPEN","sql_reopen()")) return false;

  return (sql_error_code()>=0);
}

/*
** If this is a non-cursory statement, sql_fetch() executes the statement.
** If this is a cursory statement, then it fetches a row of data.
*/
bool sql_fetch(SQL_CURSOR fd) {
  int         ret;
  SQLDA *udesc;

  if (!chk_cursor(fd, "sql_fetch"))
    return false;

  if (!ssql[fd].is_select) {
    $char *p_name = ssql[fd].p_name;
    udesc = ssql[fd].in;
    if (udesc)
      $execute $p_name using descriptor udesc;
    else
      $execute $p_name;
    ret = sql_error_code();
    if (chk_status(fd,"EXECUTE(RUN)", ssql[fd].cmd)) return false;
  } else {
    $char *c_name = ssql[fd].c_name;
    udesc = ssql[fd].out;
    if (udesc && udesc->sqld)
      $fetch $c_name using descriptor udesc;
    else
      $fetch $c_name;
    ret = sql_error_code();
    if (chk_status(fd,"FETCH(OPEN)", ssql[fd].cmd)) return false;
  }
  ssql[fd].insert_id=INSERT_ID;
  return ((ret>=0) && (ret!=100));
}

/* Convert (blob) memory to hex encoding used in UNLOAD */
static char *convert_ascii(const char *str, int len) {
  int i;
  int j;
  char *hex = "0123456789ABCDEF";
  char *p;

  p = new char [len*2+1];
  if (!p) return p;

  /* BCD to ASCII legible ! */
  for (i = j = 0; i < len; i++) {
    p[j++] = hex[((str[i] & 0xF0) >> 4)];
    p[j++] = hex[(str[i] & 0x0F)];
  }
  p[j] = '\0';
  return p;
}

SQL_ROW sql_values(SQL_CURSOR fd, int *num, int dostrip) {
  static SQL_ROW argv;
  static int maxnum;
  SQLDA *udesc;
  SQLVAR *col;
  int i;

  if (!chk_cursor(fd, "sql_values"))
    return SQL_ROW(NULL);
  if (!ssql[fd].is_select) {
    sprintf(errmsg,
            "Cannot get values for a non-select statement "
            "(fd %d) passed to sql_values", fd);
    debug(fprintf(stderr, "%s\n", errmsg));
    return SQL_ROW(NULL);
  }
  if (!ssql[fd].opened) {
    sprintf(errmsg,
            "Cannot get values for unopened statement "
            "(fd %d) passed to sql_values", fd);
    debug(fprintf(stderr, "%s\n", errmsg));
    return SQL_ROW(NULL);
  }
  udesc = ssql[fd].out;
  if (!udesc)
    return SQL_ROW(NULL);
  if (udesc->sqld > maxnum) {
    argv.argc(udesc->sqld);
    maxnum = udesc->sqld;
  }
  for (i = 0, col = (SQLVAR *)udesc->sqlvar; i < udesc->sqld; i++, col++) {
    // if something is already there, free it
    argv.clear(i);
    // build our SQL_ROW entries from what we have.
    if (col->is_blob()) {
      IFX_LOC_T *loc = (IFX_LOC_T *)col->sqldata;
      if (loc->loc_size >= 0) {
         argv[i] = new std::string(loc->loc_buffer,loc->loc_size);
      } else {
         argv[i] = new std::string(loc->loc_buffer);
      }
      if (dostrip == 1) strip(*(argv[i]));
    } else {
      argv[i] = new std::string((col->sqldata)?col->sqldata:"");
      if (dostrip == 1) strip(*(argv[i]));
    }
  }
  if (num) *num = udesc->sqld;
  return argv;
}

char *sql_command(SQL_CURSOR fd) {
  if (!chk_cursor(fd, "sql_command"))
    return NULL;
  return ssql[fd].cmd;
}

bool sql_close(SQL_CURSOR fd) {
  struct sqlda *udesc;
  struct sqlvar_struct *col;

  if (!ssql[fd].cmd)  return true;

  if (ssql[fd].is_select) {
    /* XXX - Error handling? */
    $char *c_name = ssql[fd].c_name;
    if (ssql[fd].opened)
      $close $c_name;
    $free $c_name;
  }

  {
    /* XXX - Error handling? */
    $char *p_name = ssql[fd].p_name;
    $free $p_name;
  }

  /* XXX - Valid to assert ssql[fd].cmd != 0 ? */
  if (ssql[fd].cmd && (ssql[fd].cmd != null_cmd))
    delete [] ssql[fd].cmd;

  ssql[fd].cmd = NULL;

  udesc = ssql[fd].in;
  if (udesc) {
    delete (SQLDA *)udesc; 
    udesc = NULL;
  }
  ssql[fd].in = NULL;

  udesc = ssql[fd].out;
  if (udesc) {
#ifndef FIXBUG
    // since the sqlvars for out and exec are allocated by describe
    // we need to make sure they don't get freed by us. Lets hope informix
    // is doing some garbage collecting.
    udesc->sqlvar=NULL;
#endif
    delete (SQLDA *)udesc;
    udesc = NULL;
  }

  udesc = ssql[fd].out;
  ssql[fd].out = NULL;
  /* If out and exec are the same then no need to the following */
  if (udesc != ssql[fd].exec && ssql[fd].exec)  {
    udesc = ssql[fd].exec;
#ifndef FIXBUG
    // since the sqlvars for out and exec are allocated by describe
    // we need to make sure they don't get freed by us. Lets hope informix
    // is doing some garbage collecting.
    udesc->sqlvar=NULL;
#endif
    delete (SQLDA *)udesc;
    udesc = NULL;
  }
  ssql[fd].exec = NULL;

  memset(&ssql[fd], 0, sizeof(SSQL));
  return (sql_error_code()>=0);
}

bool sql_print(SQL_CURSOR fd) {
  struct sqlda *udesc;
  struct sqlvar_struct *col;

  if (fd < 0 || fd >= sql::MAXCURSORS) {
    sprintf(errmsg, "Invalid fd %d passed to sql_print", fd);
    debug(fprintf(stderr, "%s\n", errmsg));
    return false;
  }
  printf("Command string for fd %d: %s", fd,
         (ssql[fd].cmd?ssql[fd].cmd:"null"));
  if (!ssql[fd].opened) printf(" not ");
  printf(" opened, ");
  if (!ssql[fd].opened) printf(" not ");
  printf(" selected\n");
  if (ssql[fd].in) {
    int i;
    udesc = ssql[fd].in;
    printf("Input:");
    for (i = 0, col = udesc->sqlvar; i < udesc->sqld; i++, col++) {
      if (!(col->sqlname)) col->sqlname="null";
      printf("(%s,%ld,%s)\n",col->sqlname, col->sqllen, col->sqldata);
    }
  }
  if (ssql[fd].out) {
    int i;
    udesc = ssql[fd].out;
    printf("Output:\n");
    for (i = 0, col = udesc->sqlvar; i < udesc->sqld; i++, col++) {
      if (!(col->sqlname)) col->sqlname="null";
      printf("(%s,%ld,%s)\n",col->sqlname, col->sqllen, col->sqldata);
    }
  }
  return true;
}

bool sql_database(const char *dbname, const char *password, int exclusive) {
  $char *p;
  int len;
  int fd=alloc_stmt();

  if (!dbname || *dbname == '\0') {
    p = getenv("DATABASE");
    if (!p) p = "NODATABASE";
  } else
    p = const_cast<char *>(dbname);
  if (exclusive) {
    $database $p exclusive;
    if (chk_status(fd,"DATABASE EXCLUSIVE", p)) return false;
    $set lock mode to wait;
  } else {
    $database $p;
    if (chk_status(fd,"DATABASE", p)) return false;
    $set lock mode to wait;
  }
  if (database_name) {
    delete database_name;
    database_name = NULL;
  }
  len = strlen(p) + 1;
  database_name = new char [len];
  errmalloc(database_name, "DATABASE", len);
  strcpy(database_name, p);
  sql_close(fd);
  if (SQLCODE<0) {
    return false;
  }
  return true;
}

bool sql_connect(const char *dbname, const char *uid, const char *pw, const char *cname, int with_contrans) {
  $char *p;
  $char *username;
  $char *password;
  $char *conn_name;
  int fd=alloc_stmt();

  int len;
  if (!dbname || *dbname == '\0') {
    p = getenv("DATABASE");
    if (!p) p = "NODATABASE";
  } else p = const_cast<char *>(dbname);
  conn_name = const_cast<char *>(cname);
  username = const_cast<char *>(uid);
  password = const_cast<char *>(pw);
  if (cname && *cname != '\0') {
    if (with_contrans) {
      $connect to $p as $conn_name user $username using $password
      with concurrent transaction;
    } else {
      $connect to $p as $conn_name user $username using $password;
    }
  } else {
    if (with_contrans) {
      $connect to $p user $username using $password
      with concurrent transaction;
    } else {
      $connect to $p user $username using $password;
    }
  }
  if (chk_status(fd,"CONNECT", p)) return false;
  len = strlen(p) + 1;
  if (connection_name) {
    delete connection_name;
    connection_name = NULL;
  }
  connection_name = new char [len];
  errmalloc(connection_name, "CONNECT", len);
  strcpy(connection_name, p);
  sql_close(fd);
  return (SQLCODE>=0);
}

bool sql_disconnect(const char *arg) {
  $char *cname;

  cname = const_cast<char *>(arg);
  if (!arg) {
    sprintf(errmsg, "No argument passed to sql_disconnect");
    debug(fprintf(stderr, "%s\n", errmsg));
    return false;
  }
  if (strcmp(arg, "current") == 0) {
    $disconnect current;
  } else if (strcmp(arg, "default") == 0) {
  $disconnect default;
  } else if (strcmp(arg, "all") == 0) {
    $disconnect all;
  } else {
    $disconnect $cname;
  }
  int fd=alloc_stmt();
  if (chk_status(fd,"DISCONNECT", arg)) return false;
  sql_close(fd);
  return true;
}

bool sql_sigcmd(const char *arg) {
  $char *cname;

  cname = const_cast<char *>(arg);
  if (!arg) {
    sprintf(errmsg,
            "Argument must be one of sqlbreak, sqldetach, sqlexit, sqldone");
    debug(fprintf(stderr, "%s\n", errmsg));
    return false;
  }
  if (strcmp(arg, "sqlbreak") == 0) {
    return (!sqlbreak());
  } else if (strcmp(arg, "sqldetach") == 0) {
    return !sqldetach();
  } else if (strcmp(arg, "sqlexit") == 0) {
    return !sqlexit();
  } else if (strcmp(arg, "sqldone") == 0) {
    return !sqlexit();
  } else {
    sprintf(errmsg,
            "Argument must be one of sqlbreak, sqldetach, sqlexit, sqldone");
    debug(fprintf(stderr, "%s\n", errmsg));
    return false;
  }
}

bool sql_setconnection(const char *arg, int dormant) {
  $char *cname;

  cname =  const_cast<char *>(arg);
  if (!arg) {
    sprintf(errmsg, "No argument passed to sql_setconnection");
    debug(fprintf(stderr, "%s\n", errmsg));
    return false;
  }
  if (strcmp(arg, "current") == 0) {
    if (dormant)
      $set connection current dormant;
    else
      /* NOTE: COREDUMPs occur if dormant is removed and no connection is open !!!!*/
      /* HENCE DORMANT IS ALWAYS USED */
      $set connection current dormant;
  } else if (strcmp(arg, "default") == 0) {
    if (dormant)
    $set connection default dormant;
    else
    $set connection default;
  } else {
    if (dormant)
      $set connection $cname dormant;
    else
      $set connection $cname;
  }
  int fd=alloc_stmt();
  if (dormant) {
    if (chk_status(fd,"SET CONNECTION DORMANT", arg)) return false;
  } else {
    if (chk_status(fd,"SET CONNECTION", arg)) return false;
  }
  sql_close(fd);
  return true;
}

bool sql_finish(void) {
  if (database_name) {
    delete [] database_name;
    database_name = NULL;
  }
  $close database;
  int fd=0;
  if (chk_status(fd,"CLOSE", "DATABASE")) return false;
  return true;
}

bool sql_begin(void) {
  $begin work;
  int fd=alloc_stmt();
  if (chk_status(fd,"BEGIN", "WORK")) return false;
  sql_close(fd);
  return true;
}

int sql_commit(void) {
  $commit work;
  int fd=alloc_stmt();
  if (chk_status(fd,"COMMIT", "WORK")) return false;
  sql_close(fd);
  return true;
}

int sql_rollback(void) {
  $rollback work;
  int fd=alloc_stmt();
  if (chk_status(fd,"ROLLBACK", "WORK")) return false;
  sql_close(fd);
  return true;
}

char *sql_getdatabase(void) {
  return database_name;
}

/*
sql readblob table column "rowid = NUM" intofilename [append]
    if success (0) returns NULL or SIZE of blob written
    else returns ERROR (-1)
sql writeblob table column "rowid = NUM" [fromfilename | null] [size|-1]
    if success (0) returns SIZE of blob written
    else returns ERROR (-1)
*/
int sql_readblob(const char *table, const char *column, const char *rowidclause, const char *intofile,
                 int appendmode) {
  $loc_t sloc;
  $char stmt[sql::MAXBLOB+1];
  int fd=alloc_stmt();

  if (atoi(rowidclause) > 0)
    sprintf(stmt, "select %s from %s where rowid = %s",
            column, table, rowidclause);
  else
    sprintf(stmt, "select %s from %s where %s", column, table, rowidclause);

  sloc.loc_loctype = LOCFNAME;
  sloc.loc_fname  = const_cast<char *>(intofile);

  if (appendmode)
    sloc.loc_oflags = LOC_APPEND;
  else
    sloc.loc_oflags = LOC_WONLY;

  $prepare tcl_readblob from $stmt;
  if (chk_status(fd,"PREPARE(READBLOB)", stmt)) return false;

  $declare tcl_readblob_cur cursor for tcl_readblob;
if (chk_status(fd,"DECLARE(READBLOB)", stmt)) return false;

  $open tcl_readblob_cur;
  if (chk_status(fd,"OPEN(READBLOB)", stmt)) return false;

  $fetch tcl_readblob_cur into $sloc;
  if (chk_status(fd,"FETCH(READBLOB)", stmt)) return false;

  if (sql_error_code() != 0) {
    sprintf(errmsg, "No record found.\n");
    debug(fprintf(stderr, "%s\n", errmsg));
    sql_close(fd);
    return -1;
  }

  /*
  After getting it
  sloc.loc_size # of bytes, sloc.loc_indicator = -1 for null,
  sloc.loc_status = 0 for success, -ve for error
  */
  if (sql_error_code() == 0 && sloc.loc_status == 0) {
    if (sloc.loc_indicator == -1) {
      $close tcl_readblob_cur;
      sql_close(fd);
      return -2;
    }
    $close tcl_readblob_cur;
    sql_close(fd);
    return sloc.loc_size;
  }
  $close tcl_readblob_cur;
  sql_close(fd);

  return -1;
}

int sql_writeblob(const char *table, const char *column, const char *rowidclause, const char *fromfile,
                  int fromsize) {
  $loc_t uloc;
  $char stmt[sql::MAXBLOB+1];
  $char *colname;
  int fd=alloc_stmt();

  colname = const_cast<char *>(column);

  if (atoi(rowidclause) > 0)
    sprintf(stmt, "update %s set %s = ? where rowid = %s",
            table, column, rowidclause);
  else
    sprintf(stmt, "update %s set %s = ? where %s",
            table, column, rowidclause);

  /*
  sprintf(stmt, "select 1 from %s where %s FOR UPDATE OF %s",
      table, rowidclause, column);
  */

  uloc.loc_loctype = LOCFNAME;
  uloc.loc_fname = const_cast<char *>(fromfile);
  uloc.loc_oflags = LOC_RONLY;
  uloc.loc_size  = fromsize;
  uloc.loc_indicator = 0;

  if (fromfile)
    if (strcmp(fromfile,"null") == 0 || strcmp(fromfile, "NULL") == 0)
      uloc.loc_indicator = -1;

  $prepare tcl_writeblob from $stmt;
  if (chk_status(fd,"PREPARE(WRITEBLOB)", stmt)) return false;

  $execute tcl_writeblob using $uloc;
  if (chk_status(fd,"EXECUTE(WRITEBLOB)", stmt)) return false;

  /*
  $declare tcl_writeblob_cur cursor for tcl_writeblob;
  chk_status(fd,"DECLARE(WRITEBLOB)", stmt);

  $open tcl_writeblob_cur;
  chk_status(fd,"OPEN(WRITEBLOB)", stmt);

  $fetch tcl_writeblob_cur into :found;
  chk_status(fd,"FETCH(WRITEBLOB)", stmt);

  sprintf(stmt, "update %s set %s = ? where current of tcl_writeblob_cur",
      table, column);

  $prepare tcl_writeblob_new from $stmt;
  chk_status(fd,"PREPARE UPDATE(WRITEBLOB)", stmt);

  $execute tcl_writeblob_new using $uloc;
  chk_status(fd,"EXECUTE UPDATE(WRITEBLOB)", stmt);

  */

  /*
  After updating it
  uloc.loc_size # of bytes transferred
  uloc.loc_status = 0 for success, -ve for error
  */
  sql_close(fd);
  if (sql_error_code() == 0 && uloc.loc_status == 0)
    return uloc.loc_size;

  return -1;
}

#ifdef USE_NAMESPACE
}
#endif

