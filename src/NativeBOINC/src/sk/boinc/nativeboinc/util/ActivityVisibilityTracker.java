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

import sk.boinc.nativeboinc.debug.Logging;
import android.os.Handler;
import android.util.Log;

/**
 * @author mat
 * handle event (onShow onHide) for activities group
 * mainly used to controlling wake locks for main activity
 */
public class ActivityVisibilityTracker extends Handler {

	private static final String TAG = "VisibilityTracker";
	
	private static final int ACTIVITY_VISIBLE = 1;
	private static final int ACTIVITY_PAUSED = 2;
	private static final int ACTIVITY_INVISIBLE = 3;
	
	private int mActivityState = ACTIVITY_INVISIBLE;
	
	private final static int HIDE_DELAY_PERIOD = 3000;
	
	private Runnable mDelayedHider = new Runnable() {
		@Override
		public void run() {
			if (Logging.DEBUG) Log.d(TAG, "Do hide activity");
			mActivityState = ACTIVITY_INVISIBLE;
			onHideActivity();
		}
	}; 
	
	/*
	 * trigger delayed onShow event
	 */
	public void resumeActivity() {
		if (mActivityState != ACTIVITY_VISIBLE) {
			if (Logging.DEBUG) Log.d(TAG, "Do show activity");
			mActivityState = ACTIVITY_VISIBLE;
			removeCallbacks(mDelayedHider);
			onShowActivity();
		}
	}
	
	/*
	 * trigger delayed onHide event
	 */
	public void pauseActivity() {
		if (mActivityState == ACTIVITY_VISIBLE) {
			if (Logging.DEBUG) Log.d(TAG, "Do pause activity");
			postDelayed(mDelayedHider, HIDE_DELAY_PERIOD);
			mActivityState = ACTIVITY_PAUSED;
		}
	}
	
	protected void onShowActivity() {
		// do nothing
	}
	
	protected void onHideActivity() {
		// do nothing
	}
}
