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
package sk.boinc.nativeboinc.installer;

import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.ReentrantLock;

import android.util.Log;

import sk.boinc.nativeboinc.debug.Logging;

/**
 * @author mat
 *
 */
public class InstallOpLock {
	private static final String TAG = "InstallOpLock";
	
	public static final int OP_NONE = 0;
	public static final int OP_CLIENT_INSTALL = 1;
	public static final int OP_PROJECT_INSTALL = 2;
	public static final int OP_BOINC_DUMP = 4;
	public static final int OP_BOINC_REINSTALL = 8;
	public static final int OP_BOINC_MOVETO = 16;
	public static final int OP_CLEAR_APP_INFO = 32;
	
	private int mCurrentOp = OP_NONE;
	private int mProjectInstallCount = 0;
	private ReentrantLock mLock = new ReentrantLock();
	private Condition mCondition = mLock.newCondition();
	
	public InstallOpLock() {
		
	}
	
	public void lockOp(int installOp) {
		try {
			mLock.lock();
			while (true) {
				if (installOp == OP_PROJECT_INSTALL) {
					// if nothing else than project install or client install
					if ((mCurrentOp & ~(OP_CLIENT_INSTALL | OP_PROJECT_INSTALL)) == 0) {
						if (Logging.DEBUG) Log.d(TAG, "Pass project install");
						break;
					}
				}
				if (installOp == OP_CLEAR_APP_INFO) {
					// if nothing else than clear app info
					if ((mCurrentOp & ~(OP_CLEAR_APP_INFO)) == 0) {
						if (Logging.DEBUG) Log.d(TAG, "Pass clear app info");
						break;
					}
				}
				if (mCurrentOp == OP_NONE) {
					if (Logging.DEBUG) Log.d(TAG, "Pass otherop: "+installOp);
					break;
				}
				mCondition.await();
			}
			if (installOp == OP_PROJECT_INSTALL)
				mProjectInstallCount++;
			mCurrentOp |= installOp;
		} catch(InterruptedException ex) {
		} finally {
			mLock.unlock();
		}
	}
	
	public void unlockOp(int installOp) {
		try {
			mLock.lock();
			if (installOp == OP_PROJECT_INSTALL && mProjectInstallCount > 0)
				mProjectInstallCount--;
			mCurrentOp &= ~installOp;
			mCondition.signalAll();
			if (Logging.DEBUG) Log.d(TAG, "Unlock otherop: "+installOp);
		} finally {
			mLock.unlock();
		}
	}
	
	public void unlockAll() {
		try {
			mLock.lock();
			mProjectInstallCount = 0;
			mCurrentOp = OP_NONE;
			mCondition.signalAll();
		} finally {
			mLock.unlock();
		}
	}
}
