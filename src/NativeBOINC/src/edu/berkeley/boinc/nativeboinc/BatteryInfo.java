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

package edu.berkeley.boinc.nativeboinc;

/**
 * @author mat
 *
 */
public class BatteryInfo {
	public boolean present;
	public boolean plugged;
	public double level;
	public double temperature;
	
	public BatteryInfo() {
	}
	
	public BatteryInfo(BatteryInfo info) {
		this.present = info.present;
		this.plugged = info.plugged;
		this.level = info.level;
		this.temperature = info.temperature;
	}
}
