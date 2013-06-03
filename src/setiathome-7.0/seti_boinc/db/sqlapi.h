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

/* Release 4.0  */
/* Fri Feb 27 09:06:50 EST 1998 */

#ifndef _SQLAPI_H_
#define _SQLAPI_H_
#ifdef __cplusplus

#include "sqlrow.h"
#include "sqlblob.h"
#include "sqlint8.h"

#ifdef USE_NAMESPACE
#if defined(USE_INFORMIX)
namespace ifx {
#elif defined(USE_MYSQL)
namespace mysql {
#endif
#endif

sqlint8_t sql_insert_id(SQL_CURSOR c);
/*
 * Returns the serial value (if any) of the last insert. 
 */

int sql_error_code();        // returns the current error code.
int sql_last_error_code();   // returns the last nonzero error code
void sql_reset_error_code(); // resets the last nonzero error code

const char *sql_error_message();
/*
 * Return SQL error information
 */

bool sql_database(const char *dbname, const char *password=0, int exclusive=0);
/*       connects to the database specified by dbname
 *       or if dbname is "" or cannot be opened uses
 *       the DATABASE environ variable.
 *       Parses "dbname@host" syntax.
 *       returns true if sucessful
 */

bool sql_finish();
/*        closes the database opened earlier
 */

char *sql_getdatabase();
/*        returns the database name opened by sql_database()
 */

bool sql_exists(const char *table, const char *field, const char *value, const char *where);
/*        check for existence of field in table optionally with value and
 *       optionally with a where clause where
 */

SQL_CURSOR sql_open(const char *stmt, SQL_ROW &argv=const_cast<SQL_ROW &>(empty_row));
/* open the query specified by statement and substitute all ? with
 * parameters specified
 * compiles the query and allocates space for return values
 */

extern "C" {
#endif
int begin_work();
/* starts a transaction 
 */

int commit_work();
/* commits a transaction
 */

int rollback_work();
/* rolls backs a transaction
 */
#ifdef __cplusplus
}
bool sql_close(SQL_CURSOR);
/* close the compiled query and release all memory allocated for it
 */

bool sql_fetch(SQL_CURSOR);
/* fetch a single row into the allocated space
 * use sql_values to retrieve it
 * (there is no need to free the sql_values return value!
 * the program manages this space efficiently by reallocating more
 * space if needed and does not do repeated malloc and free.)
 */

bool sql_run(const char *stmt, SQL_ROW &argv=const_cast<SQL_ROW&>(empty_row));
/*  calls sql_open, sql_fetch and then sql_close
 */

bool sql_explain(int);
/*  sets debug on for all queries if int is not 0 else turns off
 */

bool sql_reopen(SQL_CURSOR fd, SQL_ROW &argv);
/*  reopen the cursor so that fetches may be done from the
 *  start again. uses the old parameters for the open
 *  if argc is specified bind the variables again
 */

SQL_ROW sql_values(SQL_CURSOR fd, int *numvalues=0, int dostrip=1);
/*   if dostrip is 1, it will strip all trailing blanks
 */


bool chk_cursor(SQL_CURSOR fd,const char *s);
/*
 * performs an error check on the cursor
 * if NDEBUG is not defined then an error message is printed
 */

bool sql_print(SQL_CURSOR fd);
/*
 * prints implementation defined info about the query in process
 */

#ifdef USE_NAMESPACE
}
#endif
#endif
#endif
