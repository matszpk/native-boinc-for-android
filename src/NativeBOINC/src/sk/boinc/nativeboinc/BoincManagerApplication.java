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

package sk.boinc.nativeboinc;

import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.util.ArrayList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.NativeBoincTasksListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincStateListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincReplyListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
import sk.boinc.nativeboinc.nativeclient.NativeBoincUtils;
import sk.boinc.nativeboinc.util.PreferenceName;
import sk.boinc.nativeboinc.util.TaskItem;
import sk.boinc.nativeboinc.widget.NativeBoincWidgetProvider;
import sk.boinc.nativeboinc.widget.TabletWidgetProvider;
import android.app.Application;
import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.IBinder;
import android.os.Parcelable;
import android.preference.PreferenceManager;
import android.text.Html;
import android.text.util.Linkify;
import android.text.util.Linkify.TransformFilter;
import android.util.Log;
import android.widget.TextView;
import android.widget.Toast;


/**
 * Global application point which can be used by any activity.<p>
 * It handles some common stuff:
 * <ul>
 * <li>sets the default values on preferences
 * <li>provides application-wide constants
 * <li>provides basic information about application
 * </ul>
 */
public class BoincManagerApplication extends Application implements NativeBoincStateListener,
	NativeBoincReplyListener, NativeBoincTasksListener {
	
	private static final String TAG = "BoincManagerApplication";

	public static final String GLOBAL_ID = "sk.boinc.androboinc";
	public static final int DEFAULT_PORT = 31416;

	private static final int READ_BUF_SIZE = 2048;
	private static final int LICENSE_TEXT_SIZE = 37351;

	private char[] mReadBuffer = new char[READ_BUF_SIZE];
	private StringBuilder mStringBuilder = new StringBuilder(LICENSE_TEXT_SIZE);
	
	public final static String UPDATE_PROGRESS = "UPDATE_PROGRESS";
	public final static String UPDATE_TASKS = "UPDATE_TASKS";
	
	public final static int INSTALLER_NO_STAGE = 0;
	public final static int INSTALLER_CLIENT_STAGE = 1;
	public final static int INSTALLER_CLIENT_INSTALLING_STAGE = 2;
	public final static int INSTALLER_PROJECT_STAGE = 3;
	public final static int INSTALLER_PROJECT_INSTALLING_STAGE = 4;
	public final static int INSTALLER_FINISH_STAGE = 5;
	private int mInstallerStage = INSTALLER_NO_STAGE;
	
	private NativeBoincService mRunner = null;
	
	private int mWidgetUpdatePeriod = 0;
	
	private NotificationController mNotificationController = null;
	
	private boolean mDoStartClientAfterBind = false;
		
	// restart after reinstall handling
	private Object mRestartAfterReinstallSync = new Object();
	private boolean mRunRestartAfterReinstall = false;
	private boolean mRestartedAfterReinstall = false;
	
	private ServiceConnection mRunnerServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mRunner = ((NativeBoincService.LocalBinder)service).getService();
			if (Logging.DEBUG) Log.d(TAG, "runner.onServiceConnected()");
			// service automatically adds application object to listeners
			mRunner.addNativeBoincListener(BoincManagerApplication.this);
			if (!mRunner.isRun()) {
				if (mDoStartClientAfterBind && !mRunner.isRun()) {
					if (Logging.DEBUG) Log.d(TAG, "Start client after bind runner");
					mDoStartClientAfterBind = false;
					mRunner.startClient(false);
				}
			} else {
				// trigger listener
				onClientStart();
			}
		}
		
		@Override
		public void onServiceDisconnected(ComponentName name) {
			mRunner = null;
			if (Logging.DEBUG) Log.d(TAG, "runner.onServiceDisconnected()");
		}
	};
	
	private void doBindRunnerService() {
		bindService(new Intent(BoincManagerApplication.this, NativeBoincService.class),
				mRunnerServiceConnection, Context.BIND_AUTO_CREATE);
	}
	
	private void doUnbindRunnerService() {
		if (Logging.DEBUG) Log.d(TAG, "Undind runner service");
		unbindService(mRunnerServiceConnection);
		mRunner.removeNativeBoincListener(this);
		mRunner = null;
	}
	
	public void bindRunnerService() {
		if (mRunner == null)
			doBindRunnerService();
	}
	
	@Override
	public void onCreate() {
		super.onCreate();
		if (Logging.DEBUG) Log.d(TAG, "onCreate()");
		PreferenceManager.setDefaultValues(this, R.xml.manage_client, false);
		PreferenceManager.setDefaultValues(this, R.xml.preferences, false);
		
		NativeBoincUtils.killZombieClient(BoincManagerApplication.this);
		
		mNotificationController = new NotificationController(this);
		
		SharedPreferences globalPrefs = PreferenceManager.getDefaultSharedPreferences(this);
		mWidgetUpdatePeriod = Integer.parseInt(globalPrefs.getString(PreferenceName.WIDGET_UPDATE, "10"))*1000;
		mInstallerStage = globalPrefs.getInt(PreferenceName.INSTALLER_STAGE, INSTALLER_NO_STAGE);
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
	
	public void setHtmlText(TextView text, String header, String content) {
		mStringBuilder.setLength(0);
		mStringBuilder.append("<html><body>");
		mStringBuilder.append(header);
		mStringBuilder.append(":");
		mStringBuilder.append(content);
		mStringBuilder.append("</body></html>");
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
	
	public boolean isInstallerRun() {
		return mInstallerStage != INSTALLER_NO_STAGE;
	}
	
	public int getInstallerStage() {
		return mInstallerStage;
	}
	
	public void setInstallerStage(int stage) {
		mInstallerStage = stage;
		SharedPreferences globalPrefs = PreferenceManager.getDefaultSharedPreferences(this);
		
		if (Logging.DEBUG) Log.d(TAG, "Set installer stage:"+stage);
		
		int installerStageToSave = mInstallerStage;
		/* fix stage for restarted manager */
		if (mInstallerStage == INSTALLER_CLIENT_INSTALLING_STAGE)
			installerStageToSave = INSTALLER_CLIENT_STAGE;
		else if (mInstallerStage == INSTALLER_PROJECT_INSTALLING_STAGE)
			installerStageToSave = INSTALLER_PROJECT_STAGE;
		
		globalPrefs.edit().putInt(PreferenceName.INSTALLER_STAGE, installerStageToSave).commit();
	}
	
	public void backToPreviousInstallerStage() {
		if (mInstallerStage == INSTALLER_CLIENT_INSTALLING_STAGE)
			mInstallerStage = INSTALLER_CLIENT_STAGE;
		else if (mInstallerStage == INSTALLER_PROJECT_INSTALLING_STAGE)
			mInstallerStage = INSTALLER_PROJECT_STAGE;
	}
	
	public void unsetInstallerStage() {
		mInstallerStage = INSTALLER_NO_STAGE;
		SharedPreferences globalPrefs = PreferenceManager.getDefaultSharedPreferences(this);
		globalPrefs.edit().putInt(PreferenceName.INSTALLER_STAGE, mInstallerStage).commit();
	}
	
	public void runInstallerActivity(Context context) {
		switch (mInstallerStage) {
		case INSTALLER_CLIENT_STAGE:
			context.startActivity(new Intent(context, InstallStep1Activity.class));
			break;
		case INSTALLER_CLIENT_INSTALLING_STAGE:
		case INSTALLER_PROJECT_INSTALLING_STAGE:
			context.startActivity(new Intent(context, ProgressActivity.class));
			break;
		case INSTALLER_PROJECT_STAGE:
			context.startActivity(new Intent(context, InstallStep2Activity.class));
			break;
		}
	}
	
	/****
	 * runner support - 
	 * */
	
	public void bindRunnerAndStart() {
		mDoStartClientAfterBind = true;
		doBindRunnerService();
	}
	
	public NativeBoincService getRunnerService() {
		return mRunner;
	}
	
	public boolean isBoincClientRun() {
		return mRunner != null && mRunner.isRun();
	}

	@Override
	public void onClientStart() {
		if (Logging.DEBUG) Log.d(TAG, "On client start");
		Intent intent = new Intent(NativeBoincWidgetProvider.NATIVE_BOINC_WIDGET_PREPARE_UPDATE);
		PendingIntent pendingIntent = PendingIntent.getBroadcast(this, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
		try {
			pendingIntent.send();
		} catch (Exception ex) { }
		
		intent = new Intent(TabletWidgetProvider.NATIVE_BOINC_WIDGET_PREPARE_UPDATE);
		pendingIntent = PendingIntent.getBroadcast(this, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
		try {
			pendingIntent.send();
		} catch (Exception ex) { }
	}
	
	@Override
	public void onClientStop(int exitCode, boolean stoppedByManager) {
		Intent intent = new Intent(NativeBoincWidgetProvider.NATIVE_BOINC_WIDGET_UPDATE);
		PendingIntent pendingIntent = PendingIntent.getBroadcast(this, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
		try {
			pendingIntent.send();
		} catch (Exception ex) { }
		
		/* tablet widget */
		intent = new Intent(TabletWidgetProvider.NATIVE_BOINC_WIDGET_UPDATE);
		pendingIntent = PendingIntent.getBroadcast(this, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
		try {
			pendingIntent.send();
		} catch (Exception ex) { }
		
		// unbind service
		doUnbindRunnerService();
	}

	@Override
	public boolean onNativeBoincClientError(String message) {
		doUnbindRunnerService();
		// TODO: handle native boinc error
		Toast.makeText(this, message, Toast.LENGTH_LONG).show();
		return false;
	}
	

	@Override
	public boolean onNativeBoincServiceError(String message) {
		//Toast.makeText(this, message, Toast.LENGTH_LONG).show();
		return false;
	}
	
	/**
	 * 
	 * @return true if changed, otherwise false
	 */
	public boolean updateWidgetUpdatePeriod() {
		SharedPreferences globalPrefs = PreferenceManager.getDefaultSharedPreferences(this);
		int newPeriod = Integer.parseInt(globalPrefs.getString(PreferenceName.WIDGET_UPDATE, "10"))*1000;
		if (newPeriod != mWidgetUpdatePeriod) {
			mWidgetUpdatePeriod = newPeriod;
			return true;
		}
		return false;
	}
	
	public int getWigetUpdatePeriod() {
		return mWidgetUpdatePeriod;
	}

	@Override
	public void onProgressChange(double progress) {
		Intent intent = new Intent(NativeBoincWidgetProvider.NATIVE_BOINC_WIDGET_UPDATE);
		intent.putExtra(UPDATE_PROGRESS, progress);
		PendingIntent pendingIntent = PendingIntent.getBroadcast(this, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
		try {
			if (Logging.DEBUG) Log.d(TAG, "Send update intent");
			pendingIntent.send();
		} catch (Exception ex) { }
	}
	
	@Override
	public void getTasks(ArrayList<TaskItem> tasks) {
		Intent intent = new Intent(TabletWidgetProvider.NATIVE_BOINC_WIDGET_UPDATE);
		
		Parcelable[] taskItems = new Parcelable[tasks.size()];
		for (int i = 0; i < taskItems.length; i++)
			taskItems[i] = tasks.get(i);
		
		intent.putExtra(UPDATE_TASKS, taskItems);
		
		PendingIntent pendingIntent = PendingIntent.getBroadcast(this, 0, intent,
				PendingIntent.FLAG_UPDATE_CURRENT);
		try {
			if (Logging.DEBUG) Log.d(TAG, "Send update intent");
			pendingIntent.send();
		} catch (Exception ex) { }
	}
	
	/*
	 * notifications controller
	 */
	public NotificationController getNotificationController() {
		return mNotificationController;
	}

	@Override
	public void onChangeRunnerIsWorking(boolean isWorking) {
		// TODO Auto-generated method stub
	}
	
	/*
	 * restart after reinstall handling
	 */
	public boolean restartedAfterReinstall() {
		synchronized (mRestartAfterReinstallSync) {
			return mRestartedAfterReinstall;
		}
	}
	
	public void setRunRestartAfterReinstall() {
		if (Logging.DEBUG) Log.d(TAG, "SetRunRestartAfterInstall");
		synchronized(mRestartAfterReinstallSync) {
			mRunRestartAfterReinstall = true;
		}
	}
	
	public void setRestartedAfterReinstall() {
		if (Logging.DEBUG) Log.d(TAG, "SetRestartedAfterInstall");
		synchronized(mRestartAfterReinstallSync) {
			if (mRunRestartAfterReinstall)
				mRestartedAfterReinstall = true;
			mRunRestartAfterReinstall = false;
		}
	}
	
	public void resetRestartAfterReinstall() {
		if (Logging.DEBUG) Log.d(TAG, "resetRunRestartAfterInstall");
		synchronized(mRestartAfterReinstallSync) {
			mRunRestartAfterReinstall = false;
			mRestartedAfterReinstall = false;
		}
	}
}
