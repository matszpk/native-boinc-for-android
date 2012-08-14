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
package sk.boinc.nativeboinc.news;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Map;
import java.util.TimeZone;

import sk.boinc.nativeboinc.installer.ClientDistrib;
import sk.boinc.nativeboinc.installer.InstallerService;
import sk.boinc.nativeboinc.installer.ProjectDistrib;
import sk.boinc.nativeboinc.util.UpdateItem;
import sk.boinc.nativeboinc.util.VersionUtil;

import android.content.Context;

/**
 * @author mat
 *
 */
public class NewsUtil {

	public static final DateFormat sDateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm");
	
	public static final DateFormat sDisplayTimeFormat = new SimpleDateFormat("HH:mm");
	public static final DateFormat sDisplayDateFormat = new SimpleDateFormat("MM-dd HH:mm");
	public static final DateFormat sDisplayFullDateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm");
	
	/* date formatting */
	public static String formatDate(long timestamp) {
		long currentTime = System.currentTimeMillis();
		Date date = new Date(timestamp);
		if (currentTime-timestamp < 86400000)
			return sDisplayTimeFormat.format(date);
		else if (currentTime-timestamp < 365*86400000)
			return sDisplayDateFormat.format(date);
		else
			return sDisplayFullDateFormat.format(date);
	}
	
	public final static void initialize() {
		sDateFormat.setTimeZone(TimeZone.getTimeZone("UTC"));
	}
	
	public synchronized static ArrayList<NewsMessage> readNews(Context context) {
		ArrayList<NewsMessage> newsMessages = null;
		FileInputStream inStream = null;
		
		try {
			inStream = context.openFileInput("news.xml");
			newsMessages = NewsParser.parse(inStream);
		} catch(IOException ex) {
			return null;
		} finally {
			try {
				if (inStream != null)
					inStream.close();
			} catch(IOException ex) { }
		}
		
		return newsMessages;
	}
	
	public synchronized static boolean writeNews(Context context, List<NewsMessage> newsMessages) {
		// write to file
		OutputStreamWriter writer = null;
		try {
			writer = new OutputStreamWriter(context.openFileOutput("news.xml",
					Context.MODE_PRIVATE), "UTF-8");
			StringBuilder sb = new StringBuilder();
			
			sb.append("<news>\n");
			for (NewsMessage message: newsMessages) {
				sb.append("  <message>\n    <time>");
				sb.append(sDateFormat.format(new Date(message.getTimestamp())));
				sb.append("</time>\n    <title><![CDATA[");
				sb.append(message.getTitle());
				sb.append("]]></title>\n    <content><![CDATA[");
				sb.append(message.getContent());
				sb.append("]]></content>\n  </message>\n");
			}
			sb.append("</news>\n");
			writer.write(sb.toString());
			writer.flush();
		} catch(IOException ex) {
			return false;
		} finally {
			try {
				if (writer != null)
					writer.close();
			} catch(IOException ex) { }
		}
		return true;
	}
	
	public synchronized static Map<String, String> readCurrentBinaries(Context context) {
		Map<String, String> binaryVersions = null;
		FileInputStream inStream = null;
		
		try {
			inStream = context.openFileInput("current_versions.xml");
			binaryVersions = BinaryVersionsParser.parse(inStream);
		} catch(IOException ex) {
			return null;
		} finally {
			try {
				if (inStream != null)
					inStream.close();
			} catch(IOException ex) { }
		}
		
		return binaryVersions;
	}
	
	public synchronized static boolean writeCurrentBinaries(Context context,
			ClientDistrib clientDistrib, ArrayList<ProjectDistrib> projectDistribs) {
		OutputStreamWriter writer = null;
		
		try {
			writer = new OutputStreamWriter(context.openFileOutput("current_versions.xml",
					Context.MODE_PRIVATE), "UTF-8");
			
			StringBuilder sb = new StringBuilder();
			sb.append("<versions>\n  <binary>\n    <name>");
			sb.append(InstallerService.BOINC_CLIENT_ITEM_NAME);
			sb.append("</name>\n    <version>");
			sb.append(clientDistrib.version);
			sb.append("</version>\n  </binary>\n");
			
			for (ProjectDistrib distrib: projectDistribs) {
				sb.append("  <binary>\n    <name>");
				sb.append(distrib.projectName);
				sb.append("</name>\n    <version>");
				sb.append(distrib.version);
				sb.append("</version>\n  </binary>\n");
			}
			sb.append("</versions>\n");
			
			writer.write(sb.toString());
			writer.flush();
		} catch(IOException ex) {
			return false;
		} finally {
			try {
				if (writer != null)
					writer.close();
			} catch(IOException ex) { }
		}
		
		return true;
	}
	
	/**
	 * @return true if list have new binaries
	 */
	public static boolean versionsListHaveNewBinaries(Map<String, String> currentBinaries,
			UpdateItem[] updateItems) {
		for (UpdateItem item: updateItems) {
			String version = currentBinaries.get(item.name);
			if (version != null && VersionUtil.compareVersion(item.version, version) > 0)
				return true;
		}
		return false;
	}
}
