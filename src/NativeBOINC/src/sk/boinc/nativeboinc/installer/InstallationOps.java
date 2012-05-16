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

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.concurrent.Semaphore;

import sk.boinc.nativeboinc.BoincManagerApplication;
import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
import sk.boinc.nativeboinc.nativeclient.NativeBoincUtils;
import sk.boinc.nativeboinc.util.PreferenceName;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.util.Log;

/**
 * @author mat
 *
 */
public class InstallationOps {
	
	private static final String TAG = "InstallationOps";

	private Context mContext = null;
	private BoincManagerApplication mApp = null;
	
	private InstallerService.ListenerHandler mListenerHandler = null;
	
	private static final int NOTIFY_PERIOD = 400;
	private static final int BUFFER_SIZE = 4096;
	
	private String mExternalPath = null; 
	
	private NativeBoincService mRunner = null;
	
	private InstallerHandler mInstallerHandler = null;
	
	public InstallationOps(InstallerHandler installerHandler, Context context, InstallerService.ListenerHandler listenerHandler) {
		mContext = context;
		mApp = (BoincManagerApplication)mContext.getApplicationContext();
		mInstallerHandler = installerHandler;
		mListenerHandler = listenerHandler;
		mExternalPath = Environment.getExternalStorageDirectory().toString();
	}
	
	public void setRunnerService(NativeBoincService service) {
		mRunner = service;
	}
	
	private long calculateDirectorySize(File dir) {
		int length = 0;
		for (File file: dir.listFiles()) {
			if (file.isDirectory())
				length += calculateDirectorySize(file);
			else
				length += file.length();
			
			if (mDoCancelDumpFiles)
				return length;
		}
		return length;
	}
	
	private byte[] mBuffer = new byte[BUFFER_SIZE];
	
	private long mPreviousNotifyTime;
	
	private boolean mDoCancelDumpFiles = false;
	private boolean mDoCancelReinstall = false;
	
	private StringBuilder mStringBuilder = new StringBuilder();
	
	private long copyDirectory(File inDir, File outDir, String path, long copied, long inputSize)
			throws IOException {
		
		for (File file: inDir.listFiles()) {
			mStringBuilder.setLength(0);
			mStringBuilder.append(outDir.getPath());
			mStringBuilder.append("/");
			mStringBuilder.append(file.getName());
			File outFile = new File(mStringBuilder.toString());
			
			mStringBuilder.setLength(0);
			mStringBuilder.append(path);
			mStringBuilder.append("/");
			mStringBuilder.append(file.getName());
			String newPath = mStringBuilder.toString();
			
			if (file.isDirectory()) {
				if (!outFile.exists())
					outFile.mkdir();
				
				copied = copyDirectory(file, outFile, newPath, copied, inputSize);
				
				if (mDoCancelDumpFiles) // cancel
					return copied;
			} else {
				FileInputStream inStream = null;
				FileOutputStream outStream = null;
				
				try {
					inStream = new FileInputStream(file);
					outStream = new FileOutputStream(outFile);
					
					if (mDoCancelDumpFiles) // cancel
						return copied;
					
					while(true) {
						/* main copy routine */
						int readed = inStream.read(mBuffer);
						
						copied += readed;
						
						long currentNotifyTime = System.currentTimeMillis();
						if (currentNotifyTime-mPreviousNotifyTime>NOTIFY_PERIOD) {
							// notify about progress
							notifyDumpProgress((int)(10000.0*(double)copied/(double)inputSize));
							mPreviousNotifyTime = currentNotifyTime;
							
							if (mDoCancelDumpFiles) // cancel
								return copied;
						}
						
						if (readed == -1)
							break;
						outStream.write(mBuffer, 0, readed);
					}
				} finally {
					if (inStream != null)
						inStream.close();
					if (outStream != null)
						outStream.close();
				}
			}
		}
		return copied;
	}
	
	public static boolean isDestinationExists(String directory) {
		String outPath = null;
		String externalPath = Environment.getExternalStorageDirectory().toString();
		
		if (externalPath.endsWith("/")) {
			if (directory.startsWith("/"))
				outPath = externalPath+directory.substring(1);
			else
				outPath = externalPath+directory;
		} else {
			if (directory.startsWith("/"))
				outPath = externalPath+directory;
			else
				outPath = externalPath+"/"+directory;
		}
		
		return new File(outPath).exists();
	}
	
	/**
	 * main routine of dumping boinc files into sdcard
	 * @param directory
	 */
	public void dumpBoincFiles(String directory) {
		mDoCancelDumpFiles = false;
		String outPath = null;
		
		if (mExternalPath.endsWith("/")) {
			if (directory.startsWith("/"))
				outPath = mExternalPath+directory.substring(1);
			else
				outPath = mExternalPath+directory;
		} else {
			if (directory.startsWith("/"))
				outPath = mExternalPath+directory;
			else
				outPath = mExternalPath+"/"+directory;
		}
		
		if (Logging.DEBUG) Log.d(TAG, "DumpBoinc: OutPath:"+outPath);
		
		File fileDir = new File(outPath);
		deleteDirectory(fileDir);
		fileDir.mkdirs();
		
		notifyDumpBegin();
		
		File boincDir = mContext.getFileStreamPath("boinc");
		long boincDirSize = calculateDirectorySize(boincDir);
		if (mDoCancelDumpFiles) {
			notifyDumpCancel();
			mDoCancelDumpFiles = false;
			return;
		}
		
		mPreviousNotifyTime = System.currentTimeMillis();
		try {
			copyDirectory(boincDir, fileDir, "", 0, boincDirSize);
			
			if (mDoCancelDumpFiles) {
				notifyDumpCancel();
				mDoCancelDumpFiles = false;
				return;
			} else
				notifyDumpFinish();
		} catch(IOException ex) {
			notifyDumpError(ex.getMessage());
		}
		
		mDoCancelDumpFiles = false;
	}
	
	public void cancelDumpFiles() {
		mDoCancelDumpFiles = true;
	}
	
	private void deleteDirectory(File dir) {
		File[] listFiles = dir.listFiles();
		if (listFiles == null)
			return;
		
		for (File file: listFiles) {
			if (file.isDirectory())
				deleteDirectory(file);
			else
				file.delete();
			
			if (mDoCancelReinstall)
				return;
		}
		dir.delete();
	}
	
	private Semaphore mShutdownClientSem = new Semaphore(1);
	
	public void notifyShutdownClient() {
		mShutdownClientSem.release();
	}
	
	/**
	 * clean boinc files (directory),
	 */
	public void reinstallBoinc() {
		mDoCancelReinstall = false;
		
		if (mRunner.isRun()) {
			notifyReinstallOperation(mContext.getString(R.string.waitingForShutdown));
			
			mShutdownClientSem.tryAcquire();
			
			mRunner.shutdownClient();
			// awaiting for shutdown
			try {
				mShutdownClientSem.acquire();
			} catch(InterruptedException ex) {
				notifyReinstallCancel();
				mDoCancelReinstall = false;
				return;
			} finally {
				mShutdownClientSem.release();
			}
		}
		
		deleteDirectory(mContext.getFileStreamPath("boinc"));
		
		mContext.getFileStreamPath("boinc").mkdir();
		
		if (mDoCancelReinstall) {
			notifyReinstallCancel();
			mDoCancelReinstall = false;
			return;
		}
		
		// now reinitialize boinc directory (first start and other operation) */
		try {
			notifyReinstallOperation(mContext.getString(R.string.nativeClientFirstStart));
			
			if (!NativeBoincService.firstStartClient(mContext)) {
				notifyReinstallError(mContext.getString(R.string.nativeClientFirstStartError));
		    	return;
		    }
			notifyReinstallOperation(mContext.getString(R.string.nativeClientFirstKill));
			
			if (mDoCancelReinstall) {
				notifyReinstallCancel();
				return;
			}
			
			if (Logging.DEBUG) Log.d(TAG, "Adding nativeboinc to hostlist");
			// add nativeboinc to client list
	    		
	    	try {
	    		// set up waiting for benchmark 
	    		SharedPreferences sharedPrefs = PreferenceManager
	    				.getDefaultSharedPreferences(mContext);
				sharedPrefs.edit().putBoolean(PreferenceName.WAITING_FOR_BENCHMARK, true)
						.commit();
				
	    		// change hostname
	    		NativeBoincUtils.setHostname(mContext, Build.PRODUCT);
	    	} catch(IOException ex) { }
	    	
	    	if (mDoCancelReinstall) {
				notifyReinstallCancel();
				return;
			}
			
	    	// 
	    	mInstallerHandler.synchronizeInstalledProjects();
	    	
	    	mApp.setRunRestartAfterReinstall(); // inform runner
			mRunner.startClient(true);
	    	notifyReinstallFinish();
		} catch(Exception ex) {
			if (Logging.ERROR) Log.e(TAG, "on client install finishing:"+
					ex.getClass().getCanonicalName()+":"+ex.getMessage());
			notifyReinstallError(mContext.getString(R.string.unexpectedError)+": "+ex.getMessage());
		} finally {
			mDoCancelReinstall = false;
		}
	}
	
	public void cancelReinstall() {
		mDoCancelReinstall = true;
	}
	
	
	/**
	 * Dump boinc files notify routines
	 */
	private synchronized void notifyDumpBegin() {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onOperation(InstallerService.BOINC_DUMP_ITEM_NAME, "",
						mContext.getString(R.string.dumpBoincBegin));
			}
		});
	}
	
	private synchronized void notifyDumpProgress(final int progress) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onOperationProgress(InstallerService.BOINC_DUMP_ITEM_NAME, "",
						mContext.getString(R.string.dumpBoincProgress), progress);
			}
		});
	}
	
	private synchronized void notifyDumpCancel() {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onOperationCancel(InstallerService.BOINC_DUMP_ITEM_NAME, "");
			}
		});
	}
	
	private synchronized void notifyDumpError(final String error) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onOperationError(InstallerService.BOINC_DUMP_ITEM_NAME, "",
						mContext.getString(R.string.dumpBoincError, error));
			}
		});
	}
	
	private synchronized void notifyDumpFinish() {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onOperationFinish(InstallerService.BOINC_DUMP_ITEM_NAME, "");
			}
		});
	}
	
	/**
	 * Reinstall boinc notifying routines
	 */
	private synchronized void notifyReinstallOperation(final String opDescription) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onOperation(InstallerService.BOINC_REINSTALL_ITEM_NAME, "",
						opDescription);
			}
		});
	}
	
	private synchronized void notifyReinstallCancel() {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onOperationCancel(InstallerService.BOINC_REINSTALL_ITEM_NAME, "");
			}
		});
	}
	
	private synchronized void notifyReinstallError(final String error) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onOperationError(InstallerService.BOINC_REINSTALL_ITEM_NAME, "",
						mContext.getString(R.string.dumpBoincError, error));
			}
		});
	}
	
	private synchronized void notifyReinstallFinish() {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onOperationFinish(InstallerService.BOINC_REINSTALL_ITEM_NAME, "");
			}
		});
	}
}
