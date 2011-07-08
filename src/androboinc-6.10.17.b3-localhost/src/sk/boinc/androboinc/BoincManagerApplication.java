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

package sk.boinc.androboinc;

import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import sk.boinc.androboinc.debug.Logging;
import sk.boinc.androboinc.util.PreferenceName;
import android.app.Application;
import android.content.SharedPreferences;
import android.content.pm.PackageManager.NameNotFoundException;
import android.preference.PreferenceManager;
import android.text.Html;
import android.text.util.Linkify;
import android.text.util.Linkify.TransformFilter;
import android.util.Log;
import android.widget.TextView;


/**
 * Global application point which can be used by any activity.<p>
 * It handles some common stuff:
 * <ul>
 * <li>sets the default values on preferences
 * <li>provides application-wide constants
 * <li>provides basic information about application
 * </ul>
 */
public class BoincManagerApplication extends Application {
	private static final String TAG = "BoincManagerApplication";

	public static final String GLOBAL_ID = "sk.boinc.androboinc";
	public static final int DEFAULT_PORT = 31416;

	private static final int READ_BUF_SIZE = 2048;
	private static final int LICENSE_TEXT_SIZE = 37351;

	private char[] mReadBuffer = new char[READ_BUF_SIZE];
	private StringBuilder mStringBuilder = new StringBuilder(LICENSE_TEXT_SIZE);

	@Override
	public void onCreate() {
		super.onCreate();
		if (Logging.DEBUG) Log.d(TAG, "onCreate()");
		PreferenceManager.setDefaultValues(this, R.xml.manage_client, false);
		PreferenceManager.setDefaultValues(this, R.xml.preferences, false);
	}

	@Override
	public void onTerminate() {
		super.onTerminate();
		if (Logging.DEBUG) Log.d(TAG, "onTerminate() - finished");
	}

	@Override
	public void onLowMemory() {
		if (Logging.DEBUG) Log.d(TAG, "onLowMemory()");
		// Let's free what we do not need essentially
		mStringBuilder = null; // So garbage collector will free the memory
		mReadBuffer = null;
		super.onLowMemory();
	}

	/**
	 * Finds whether this is the application was upgraded recently.
	 * It also marks the status in preferences, so even if first call of this
	 * method after upgrade returns true, all subsequent calls will return false.
	 * 
	 * @return true if this is first call of this method after application upgrade, 
	 *         false if this application version was already run previously
	 */
	public boolean getJustUpgradedStatus() {
		SharedPreferences globalPrefs = PreferenceManager.getDefaultSharedPreferences(this);
		int upgradeInfoShownVersion = globalPrefs.getInt(PreferenceName.UPGRADE_INFO_SHOWN_VERSION, 0);
		int currentVersion = 0;
		try {
			currentVersion = getPackageManager().getPackageInfo(getPackageName(), 0).versionCode;
		}
		catch (NameNotFoundException e) {
			if (Logging.ERROR) Log.e(TAG, "Cannot retrieve application version");
			return false;
		}
		if (Logging.DEBUG) Log.d(TAG, "currentVersion=" + currentVersion + ", upgradeInfoShownVersion=" + upgradeInfoShownVersion);
		if (currentVersion == upgradeInfoShownVersion) {
			// NOT upgraded, we shown info already
			return false;
		}
		// Just upgraded; mark the status in preferences
		if (Logging.DEBUG) Log.d(TAG, "First run after upgrade: currentVersion=" + currentVersion + ", upgradeInfoShownVersion=" + upgradeInfoShownVersion);
		SharedPreferences.Editor editor = globalPrefs.edit();
		editor.putInt(PreferenceName.UPGRADE_INFO_SHOWN_VERSION, currentVersion).commit();
		return true;
	}

	public String getApplicationVersion() {
		if (mStringBuilder == null) mStringBuilder = new StringBuilder(32);
		mStringBuilder.setLength(0);
		mStringBuilder.append(getString(R.string.app_name));
		mStringBuilder.append(" v");
		try {
			mStringBuilder.append(getPackageManager().getPackageInfo(getPackageName(), 0).versionName);
		}
		catch (NameNotFoundException e) {
			if (Logging.ERROR) Log.e(TAG, "Cannot retrieve application version");
			mStringBuilder.setLength(mStringBuilder.length() - 2); // Truncate " v" set above
		}		
		return mStringBuilder.toString();
	}

	public void setAboutText(TextView text) {
		text.setText(getString(R.string.aboutText, getApplicationVersion()));
		String httpURL = "http://";
		// Link to BOINC.SK page
		Pattern boincskText = Pattern.compile("BOINC\\.SK");
		TransformFilter boincskTransformer = new TransformFilter() {
			@Override
			public String transformUrl(Matcher match, String url) {
				return url.toLowerCase() + "/";
			}
		};
		Linkify.addLinks(text, boincskText, httpURL, null, boincskTransformer);
		// Link to GPLv3 license
		Pattern gplText = Pattern.compile("GPLv3");
		TransformFilter gplTransformer = new TransformFilter() {
			@Override
			public String transformUrl(Matcher match, String url) {
				return "www.gnu.org/licenses/gpl-3.0.txt";
			}
		};
		Linkify.addLinks(text, gplText, httpURL, null, gplTransformer);
	}

	public void setLicenseText(TextView text) {
		text.setText(Html.fromHtml(readRawText(R.raw.license)));
		Linkify.addLinks(text, Linkify.ALL);
	}

	public void setChangelogText(TextView text) {
		String changelog = readRawText(R.raw.changelog);
		// Transform plain-text ChangeLog to simple HTML format:
		// 1. Make line beginning with "Version" bold
		String trans1 = changelog.replaceAll("(?m)^([Vv]ersion.*)$", "<b>$1</b>");
		// 2. Append <br> at the end of each line
		String trans2 = trans1.replaceAll("(?m)^(.*)$", "$1<br>");
		// 3. Add HTML tags
		if (mStringBuilder == null) mStringBuilder = new StringBuilder(32);
		mStringBuilder.setLength(0);
		mStringBuilder.append("<html>\n<body>\n");
		mStringBuilder.append(trans2);
		mStringBuilder.append("\n</body>\n</html>");
		text.setText(Html.fromHtml(mStringBuilder.toString()));
	}

	public String readRawText(final int resource) {
		InputStream inputStream = null;
		if (mReadBuffer == null) mReadBuffer = new char[READ_BUF_SIZE];
		if (mStringBuilder == null) mStringBuilder = new StringBuilder(LICENSE_TEXT_SIZE);
		mStringBuilder.ensureCapacity(LICENSE_TEXT_SIZE);
		mStringBuilder.setLength(0);
		try {
			inputStream = getResources().openRawResource(resource);
			Reader reader = new InputStreamReader(inputStream, "UTF-8");
			int bytesRead;
			do {
				bytesRead = reader.read(mReadBuffer);
				if (bytesRead == -1) break;
				mStringBuilder.append(mReadBuffer, 0, bytesRead);
			} while (true);
			inputStream.close();
		}
		catch (IOException e) {
			e.printStackTrace();
			if (Logging.ERROR) Log.e(TAG, "Error when reading raw resource " + resource);
		}
		return mStringBuilder.toString();
	}
}
