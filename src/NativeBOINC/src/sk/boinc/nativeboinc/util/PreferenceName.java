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
	public static final String POWER_SAVING = "powerSaving";
	public static final String AUTO_CONNECT = "autoConnect";
	public static final String AUTO_CONNECT_HOST = "autoConnectHost";
	public static final String AUTO_UPDATE_WIFI = "autoUpdateIntervalWiFi";
	public static final String AUTO_UPDATE_MOBILE = "autoUpdateIntervalMobile";
	public static final String AUTO_UPDATE_LOCALHOST = "autoUpdateIntervalLocalhost";
	public static final String WIDGET_UPDATE = "widgetUpdateInterval";
	public static final String SCREEN_LOCK_UPDATE = "screenLockUpdateInterval";
	public static final String COLLECT_STATS = "trackNetworkUsage";
	public static final String UPGRADE_INFO_SHOWN_VERSION = "upgradeInfoShownVersion";
	public static final String LAST_ACTIVE_TAB = "lastActiveTab";
	public static final String LIMIT_MESSAGES = "limitMessages";
	public static final String LAST_MESSAGES_FILTER = "lastMessagesFilter";
	
	/* for client */
	public static final String CLIENT_VERSION = "clientVersion";
	/* installation */
	public static final String INSTALLER_STAGE = "installerStage";
	public static final String WAITING_FOR_BENCHMARK = "waitingForBenchmark";
	
	/* native client */
	public static final String NATIVE_AUTOSTART = "nativeAutostart";
	public static final String NATIVE_HOSTNAME = "nativeHostname";
	public static final String NATIVE_REMOTE_ACCESS = "nativeRemoteAccess";
	public static final String NATIVE_ACCESS_PASSWORD = "nativeAccessPassword";
	public static final String NATIVE_HOST_LIST = "nativeAccessList";
	public static final String NATIVE_INSTALLED_BINARIES = "nativeInstalledBinaries";
	public static final String NATIVE_UPDATE_BINARIES = "nativeUpdateBinaries";
	public static final String NATIVE_UPDATE_FROM_SDCARD = "nativeUpdateFromSDCard";
	public static final String NATIVE_DUMP_BOINC_DIR = "nativeDumpBoincDir";
	public static final String NATIVE_REINSTALL = "nativeReinstall";
	public static final String NATIVE_SHOW_LOGS = "nativeShowLogs";
	
	/* news receiver */
	public static final String LATEST_NEWS_TIME = "latestNewsTime";
}
