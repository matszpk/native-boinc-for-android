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
	public final static int BOINC_CLIENT_RUN = 1;
	public final static int CONN_MANAGER_POLL = 2;
	public final static int NATIVEBOINC_NEWS = 3;
	public final static int ACCOUNT_MGR_SYNC = 4;
	public final static int BENCHMARK_RUN = 5;
	public final static int INSTALL_PROJECT_APPS_BASE = 100;
}
