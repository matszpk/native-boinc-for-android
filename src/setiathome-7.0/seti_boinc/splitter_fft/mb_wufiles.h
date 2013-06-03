/*
 *
 * Functions for managing wufiles and their data
 *
 * $Id: mb_wufiles.h,v 1.1.2.2 2007/04/25 17:20:28 korpela Exp $
 *
 */


extern  std::vector<std::vector<unsigned char> > bin_data;

int make_wu_headers(std::vector<dr2_compact_block_t> &tapebuffer, telescope_id tel, 
    std::vector<workunit> &wuheader) ;
void write_wufile_blocks(int nbytes) ;
void rename_wu_files();

/*
 * $Log: mb_wufiles.h,v $
 * Revision 1.1.2.2  2007/04/25 17:20:28  korpela
 * *** empty log message ***
 *
 * Revision 1.1.2.1  2006/12/14 22:24:46  korpela
 * *** empty log message ***
 *
 * Revision 1.2  2003/08/05 17:23:45  korpela
 * More work on database stuff.
 * Further tweaks.
 *
 * Revision 1.1  2003/06/03 00:23:44  korpela
 *
 * Again
 *
 * Revision 3.0  2001/08/01 19:04:57  korpela
 * Check this in before Paul screws it up.
 *
 * Revision 2.1  1998/11/02 16:41:21  korpela
 * Minor Change.
 *
 * Revision 2.0  1998/10/30  22:00:04  korpela
 * Conversion to C++ and merger with client source tree.
 *
 * Revision 1.1  1998/10/27  01:13:16  korpela
 * Initial revision
 *
 *
 */
