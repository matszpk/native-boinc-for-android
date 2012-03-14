/* 
 * NativeBOINC - Native BOINC Client with Manager
 * Copyright (C) 2011, Mateusz Szpakowski
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */
package sk.boinc.nativeboinc.bridge;

import edu.berkeley.boinc.lite.Notice;
import sk.boinc.nativeboinc.clientconnection.NoticeInfo;

/**
 * @author mat
 *
 */
public class NoticeInfoCreator {
	public static NoticeInfo create(final Notice notice, final Formatter formatter) {
		NoticeInfo noticeInfo = new NoticeInfo();
		noticeInfo.seqNo = notice.seqno;
		noticeInfo.category = notice.category;
		noticeInfo.title_project = notice.project_name + ": " + notice.title;
		noticeInfo.title = notice.title;
		noticeInfo.description = formatter.toHtmlContent(notice.description);
		noticeInfo.link_html = formatter.toHtmlLink(notice.link);
		noticeInfo.link = notice.link;
		noticeInfo.create_time = formatter.formatDate((long)notice.create_time);
		return noticeInfo;
	}
}
