/*
 *
 * timecvt.c
 *
 * Time conversion routines.
 *
 * $Id: hr_min_sec.h,v 1.1 2003/06/03 01:01:16 korpela Exp $
 *
 */

char* hr_min_sec (double x) ;


/*
 * $Log: hr_min_sec.h,v $
 * Revision 1.1  2003/06/03 01:01:16  korpela
 *
 * First working splitter under CVS.
 *
 * Revision 4.4  2003/03/10 02:50:03  jeffc
 * jeffc - t_strncpy()
 *
 * Revision 4.3  2003/01/22 20:51:58  korpela
 * Changed all strcpy to strncpy.
 * Changed all gets to fgets.
 *
 * Revision 4.2  2001/12/27 21:35:57  davea
 * *** empty log message ***
 *
 * Revision 4.1  2001/09/10 00:25:17  davea
 * *** empty log message ***
 *
 * Revision 4.0  2000/10/05 18:12:12  korpela
 * Synchronized versions to 4.0 following release of 3.0 client
 *
 * Revision 3.8  2000/09/14 01:01:31  korpela
 * *** empty log message ***
 *
 * Revision 3.7  2000/04/24 08:39:06  charlief
 * eliminate compiler warning in hr_min_sec() function.
 *
 * Revision 3.6  2000/01/19 08:32:32  davea
 * *** empty log message ***
 *
 * Revision 3.5  1999/10/19 08:07:24  davea
 * *** empty log message ***
 *
 * Revision 3.4  1999/06/26 06:56:44  hiramc
 * Hiram 99/06/25 23:59 moved the include <string.h> out of the
 * ifdef _WIN32 to define strcpy() for all compiles
 *
 * Revision 3.3  1999/06/22 09:40:36  charlief
 * Reverse last change:restore jd_string() as it was before.
 * Add new function short_jd_string(), which does not print
 * Julian Date as a floating point value.
 *
 * Revision 3.1  1999/06/10 00:44:11  korpela
 * *** empty log message ***
 *
 * Revision 3.0  1999/05/14 19:17:35  korpela
 * 1.0(Win/Mac) 1.1(Unix) Release Version
 *
 * Revision 2.11  1999/03/14 00:23:02  davea
 * *** empty log message ***
 *
 * Revision 2.10  1999/03/11 21:46:58  korpela
 * Fixed rounding error in st_to_ut().
 *
 * Revision 2.9  1999/03/05  09:15:07  kyleg
 * *** empty log message ***
 *
 * Revision 2.8  1999/02/18  19:36:49  korpela
 * *** empty log message ***
 *
 * Revision 2.7  1999/02/13  23:54:44  kyleg
 * *** empty log message ***
 *
 * Revision 2.6  1999/02/11 08:36:54  davea
 * *** empty log message ***
 *
 * Revision 2.5  1998/11/17 21:47:02  korpela
 * *** empty log message ***
 *
 * Revision 2.4  1998/11/05  21:25:21  davea
 * replace floor() with (int)
 *
 * Revision 2.3  1998/11/05 02:00:06  kyleg
 * *** empty log message ***
 *
 * Revision 2.2  1998/11/02 17:59:06  korpela
 * *** empty log message ***
 *
 * Revision 2.1  1998/11/02  17:23:29  korpela
 * .`
 *
 * Revision 2.0  1998/10/30  22:00:04  korpela
 * Conversion to C++ and merger with client source tree.
 *
 * Revision 1.2  1998/10/27  00:58:56  korpela
 * Bug fixes.
 *
 * Revision 1.1  1998/10/19  18:57:56  korpela
 * Initial revision
 *
 *
 */
