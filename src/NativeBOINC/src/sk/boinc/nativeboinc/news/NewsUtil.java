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
import java.util.List;

import android.content.Context;

/**
 * @author mat
 *
 */
public class NewsUtil {

	public static final DateFormat sDateFormat = new SimpleDateFormat("YYYY-MM-DD HH:mmZ");
	
	public static ArrayList<NewsMessage> readNews(Context context) {
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
	
	public static final boolean writeNews(Context context, List<NewsMessage> newsMessages) {
		// write to file
		OutputStreamWriter writer = null;
		try {
			writer = new OutputStreamWriter(context.openFileOutput("installed_distribs.xml",
					Context.MODE_PRIVATE), "UTF-8");
			StringBuilder sb = new StringBuilder();
			
			sb.append("<news>\n");
			for (NewsMessage message: newsMessages) {
				sb.append("  <message>\n    <timestamp>");
				sb.append(message.getFormattedTime());
				sb.append("</timestamp>\n    <title><![CDATA[");
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
}
