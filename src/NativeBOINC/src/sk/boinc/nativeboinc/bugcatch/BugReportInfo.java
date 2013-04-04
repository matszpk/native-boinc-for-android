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
package sk.boinc.nativeboinc.bugcatch;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;

public class BugReportInfo {
	private long mReportId;
	private String mTime;
	private String mContent; // only 10 lines of content (excluding header)
	
	private static final DateFormat sDisplayTimeFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss"); 
	
	public BugReportInfo(long id, String content) {
		this.mReportId = id;
		this.mTime = sDisplayTimeFormat.format(new Date(id));
		this.mContent = content;
	}
	
	public long getId() {
		return mReportId;
	}
	
	public String getTime() {
		return mTime;
	}
	
	public String getContent() {
		return mContent;
	}
}
