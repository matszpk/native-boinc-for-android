/* 
 * AndroBOINC - BOINC Manager for Android
 * Copyright (C) 2010, Pavol Michalec
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

/*
 * The values of following constants must be consistent with the corresponding keys 
 * in file preferences.xml (res/xml directory)
 */
public interface PreferenceName {
	public static final String SCREEN_ORIENTATION = "screenOrientation";
	public static final String LOCK_SCREEN_ON = "lockScreenOn";
	public static final String AUTO_CONNECT = "autoConnect";
	public static final String AUTO_CONNECT_HOST = "autoConnectHost";
	public static final String AUTO_UPDATE_WIFI = "autoUpdateIntervalWiFi";
	public static final String AUTO_UPDATE_MOBILE = "autoUpdateIntervalMobile";
	public static final String AUTO_UPDATE_LOCALHOST = "autoUpdateIntervalLocalhost";
	public static final String COLLECT_STATS = "trackNetworkUsage";
	public static final String UPGRADE_INFO_SHOWN_VERSION = "upgradeInfoShownVersion";
	public static final String LAST_ACTIVE_TAB = "lastActiveTab";
	public static final String LIMIT_MESSAGES = "limitMessages";
	
	/* for client */
	public static final String CLIENT_VERSION = "clientVersion";
}
