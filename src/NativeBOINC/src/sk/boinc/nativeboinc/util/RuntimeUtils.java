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

import java.io.File;

/**
 * @author mat
 *
 */
public class RuntimeUtils {

	private static int sCpusCount = -1;
	
	public static int getRealCPUCount() {
		if (sCpusCount == -1) {
			int cpusCount = Runtime.getRuntime().availableProcessors();
			
			for (int i = 0; i < 17; i++) {
				File cpuFile = new File("/sys/devices/system/cpu/cpu"+i);
				if (!cpuFile.exists())
					return Math.max(cpusCount, i);
			}
			sCpusCount = Math.max(cpusCount, 17);
		}
		
		return sCpusCount;
	}
}
