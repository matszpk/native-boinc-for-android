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

package sk.boinc.nativeboinc.util;

import java.util.ArrayList;
import java.util.Iterator;

public class StringUtil {

	public final static String joinString(String delimiter, ArrayList<String> messages) {
		if (messages == null || delimiter == null)
			return null;
		
		StringBuilder builder = new StringBuilder();
		Iterator<String> iter = messages.iterator();
		
		while(iter.hasNext()) {
			builder.append(iter.next());
			if (iter.hasNext())
				builder.append(delimiter);
		}
		return builder.toString();
	}
	
	public final static String normalizeHttpUrl(String url) {
		String out = url.trim();
		/* check http or https */
		if (!url.startsWith("http://") && !url.startsWith("https://"))
			out = "http://" + url;
		if (!url.endsWith("/"))
			out = out+"/";
		
		return out;
	}
}
