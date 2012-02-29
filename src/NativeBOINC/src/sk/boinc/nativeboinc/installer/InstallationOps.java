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

import sk.boinc.nativeboinc.R;

import android.content.Context;
import android.os.Environment;

/**
 * @author mat
 *
 */
public class InstallationOps {

	private Context mContext = null;
	
	private InstallerService.ListenerHandler mListenerHandler = null;
	
	private static final int NOTIFY_PERIOD = 400;
	
	private String mExternalPath = null; 
	
	public InstallationOps(Context context, InstallerService.ListenerHandler listenerHandler) {
		mContext = context;
		mListenerHandler = listenerHandler;
		mExternalPath = Environment.getExternalStorageDirectory().toString();
	}
	
	private static final int BUFFER_SIZE = 4096;
	
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
	
	private long copyDirectory(File inDir, File outDir, String path, long copied, long inputSize)
			throws IOException {
		
		for (File file: inDir.listFiles()) {
			File outFile = new File(outDir.getPath()+"/"+file.getName());
			String newPath = path+"/"+file.getName();
			
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
		
		File fileDir = new File(outPath);
		if (!fileDir.exists()) // create directory before 
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
		for (File file: dir.listFiles()) {
			if (file.isDirectory())
				deleteDirectory(file);
			else
				file.delete();
		}
	}
	
	/**
	 * clean boinc files (directory),
	 */
	public void clearBoincFiles() {
		deleteDirectory(mContext.getFileStreamPath("boinc"));
		// now reinitialize boinc directory (first start and other operation) */
	}
	
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
}
