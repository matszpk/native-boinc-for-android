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
public interface NotificationId {
	public final static int INSTALL_BOINC_CLIENT = 0;
	public final static int NATIVE_BOINC_SERVICE = 1;
	public final static int BOINC_CLIENT_EVENT = 2;
	public final static int INSTALL_DUMP_FILES = 3;
	public final static int INSTALL_REINSTALL = 4;
	public final static int NATIVE_BOINC_NEWS = 5;
	public final static int NATIVE_NEW_BINARIES = 6;
	public final static int MOVE_INSTALLATION_TO = 7;
	public final static int BUG_DETECTED = 8;
	public final static int BUG_CATCHER_PROGRESS = 9;
	public final static int BUG_CATCHER_FAIL = 10;
	public final static int INSTALL_PROJECT_APPS_BASE = 100;
}
