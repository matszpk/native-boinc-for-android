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

import java.io.IOException;
import java.util.Collection;

/**
 * @author mat
 *
 */
public class SDCardExecs {
	public final native static int openExecsLock(String execsname, boolean isWrite) throws IOException;
	
	public final native static void readExecs(int fd, Collection<String> coll) throws IOException;
	public final native static boolean checkExecMode(int fd, String filename) throws IOException;
	public final native static void setExecMode(int fd, String filename,
			boolean execMode) throws IOException;
	
	public final native static void closeExecsLock(int fd);
	
	static {
		System.loadLibrary("nativeboinc_utils");
	}
}
