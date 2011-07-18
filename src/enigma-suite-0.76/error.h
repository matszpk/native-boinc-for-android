#ifndef ERROR_H
#define ERROR_H

enum { ERR_A_ONLY, ERR_EXCL_A, ERR_RING_SHORTCUT };

void err_alloc_fatal(const char *s);
void err_open_fatal(const char *s);
void err_open_fatal_resume(const char *s);
void err_stream_fatal(const char *s);
void err_illegal_char_fatal(const char *s);
void err_sigaction_fatal(int signum);
void err_input_fatal(int type);

/* log message */
void hillclimb_log(const char *s);

#endif


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
