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

/**
 * @author mat
 *
 */
public class FileUtils {
	public final static String joinBaseAndPath(String base, String filePath) {
		String outPath;
		if (base.endsWith("/")) {
			if (filePath.startsWith("/"))
				outPath = base+filePath.substring(1);
			else
				outPath = base+filePath;
		} else {
			if (filePath.startsWith("/"))
				outPath = base+filePath;
			else
				outPath = base+"/"+filePath;
		}
		return outPath;
	}
}
