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

package sk.boinc.nativeboinc.nativeclient;

import sk.boinc.nativeboinc.R;
import android.content.res.Resources;

/**
 * @author mat
 *
 */
public class CpuType {
	public static final int ARMV6 = 0;
	public static final int ARMV6_VFP = 1;
	public static final int ARMV7_NEON = 2;
	
	public static int parseCpuType(String cpuString) {
		if (cpuString.equals("ARMv6")) {
			return CpuType.ARMV6;
		} else if (cpuString.equals("ARMv6 VFP")) {
			return CpuType.ARMV6_VFP;
		} else if (cpuString.equals("ARMv7 NEON")) {
			return CpuType.ARMV7_NEON;
		}  else
			throw new RuntimeException("Invalid Cpu type");
	}
	
	public static String getCpuDisplayName(Resources res, int cpuType) {
		switch(cpuType) {
		case ARMV6:
			return res.getString(R.string.cpuTypeARMv6);
		case ARMV6_VFP:
			return res.getString(R.string.cpuTypeARMv6VFP);
		case ARMV7_NEON:
			return res.getString(R.string.cpuTypeARMv7NEON);
		default:
			return res.getString(R.string.cpuTypeUnknown);
		}
	}
	
	public static String getCpuName(int cpuType) {
		switch(cpuType) {
		case ARMV6:
			return "ARMv6";
		case ARMV6_VFP:
			return "ARMv6 VFP";
		case ARMV7_NEON:
			return "ARMv7 NEON";
		default:
			return "Unknown";
		}
	}
}
