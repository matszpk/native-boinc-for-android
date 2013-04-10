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
package sk.boinc.nativeboinc.bugcatch;

import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.InterruptedIOException;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;

import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.util.FileUtils;

import android.net.ParseException;
import android.os.Environment;
import android.os.Handler;
import android.util.Log;

/**
 * @author mat
 *
 */
public class BugCatcherHandler extends Handler {

	private static final String TAG = "BugCatcherHandler";
	
	private BugCatcherService mBugCatcher = null;
	private BugCatcherService.ListenerHandler mListenerHandler = null;
	private BugCatcherThread mBugCatcherThread = null;
	
	private boolean mIsWorking = false;
	
	public BugCatcherHandler(BugCatcherService service, BugCatcherService.ListenerHandler listenerHandler,
			BugCatcherThread thread) {
		mBugCatcher = service;
		mListenerHandler = listenerHandler;
		mBugCatcherThread = thread;
	}
	
	public synchronized void cancelOperation() {
		if (mIsWorking) {
			if (mBugCatcherThread != null)
				mBugCatcherThread.interrupt(); // interrupts
		}
	}
	
	public void destroy() {
		mBugCatcher = null;
		mListenerHandler = null;
		mBugCatcherThread = null;
	}
	
	public synchronized boolean isWorking() {
		return mIsWorking;
	}
	
	private List<File> filterBugReportContentFiles(File[] inFiles) {
		List<File> outFiles = new ArrayList<File>(1); 
		for (File file: inFiles) {
			String fname = file.getName();
			if (fname.endsWith("_content.txt"))
				outFiles.add(file);
		}
		return outFiles;
	}
	
	public void saveToSDCard(String bugSDCardDir) {
		notifyWorking(true);
		File bugCatchDir = new File(mBugCatcher.getFilesDir().getAbsolutePath()+"/bugcatch");
		
		if (!bugCatchDir.exists()) {
			if (Logging.INFO) Log.i(TAG, "BugCatcher directory doesnt exists");
			notifyBugReportFinish(BugCatchOp.BugsToSDCard,
					mBugCatcher.getString(R.string.bugCopyingFinished));
			notifyWorking(false);
			return;
		}
		
		StringBuilder inPathSB = new StringBuilder();
		inPathSB.append(bugCatchDir.getAbsolutePath());
		inPathSB.append('/');
		int inBugDirLen = inPathSB.length();
		/* prepare outpath */
		StringBuilder outPathSB = new StringBuilder();
		outPathSB.append(FileUtils.joinBaseAndPath(
				Environment.getExternalStorageDirectory().getAbsolutePath(),
				bugSDCardDir));
		if (outPathSB.charAt(outPathSB.length()-1) != '/')
			outPathSB.append('/');
		
		File outBugCatchDir = new File(outPathSB.toString());
		if (!outBugCatchDir.exists())
			outBugCatchDir.mkdirs();
		
		int outBugDirLen = outPathSB.length();
		
		notifyBugReportBegin(BugCatchOp.BugsToSDCard, mBugCatcher.getString(R.string.bugCopyingBegin));
		
		byte[] buffer = new byte[1024];
		BugCatcherService.openLockForRead();
		File[] inFiles = bugCatchDir.listFiles();
		BugCatcherService.closeLockForRead();
		
		List<File> filteredFiles = filterBugReportContentFiles(inFiles);
		int count = 0;
		int total = filteredFiles.size()*2;
		
		FileInputStream inStream = null;
		FileOutputStream outStream = null;
		long bugReportId = 0;
		
		try {
			for (File file: filteredFiles) {
				String fname = file.getName();
				String idString = null;
				if (fname.endsWith("_content.txt"))
					idString = fname.substring(0, fname.length()-12);
				
				try {
					bugReportId = Long.parseLong(idString);
				} catch(ParseException ex) {
					if (Logging.WARNING) Log.w(TAG, "Cant parse bugReportId");
					count++;
					continue;
				}
				
				if (Logging.DEBUG) Log.d(TAG, "Copying bugreport "+bugReportId);
				
				notifyBugReportProgress(BugCatchOp.BugsToSDCard,
						mBugCatcher.getString(R.string.bugCopyingProgress),
						bugReportId, count, total);
				count++;
				
				if (Thread.interrupted()) {
					notifyBugReportCancel(BugCatchOp.BugsToSDCard,
							mBugCatcher.getString(R.string.bugCopyingCancel));
					return;
				}
				
				inStream = new FileInputStream(file);
				outPathSB.delete(outBugDirLen, outPathSB.length());
				outPathSB.append(fname);
				outStream = new FileOutputStream(outPathSB.toString());
				
				while (true) {
					int readed = inStream.read(buffer);
					if (readed == -1)
						break;
					outStream.write(buffer, 0, readed);
				}
				
				// delay
				/*try {
					Thread.sleep(2000);
				} catch(InterruptedException ex) {
					notifyBugReportCancel(BugCatchOp.BugsToSDCard,
							mBugCatcher.getString(R.string.bugCopyingCancel));
					return;
				}*/
				
				if (Thread.interrupted()) {
					notifyBugReportCancel(BugCatchOp.BugsToSDCard,
							mBugCatcher.getString(R.string.bugCopyingCancel));
					return;
				}
				outStream.flush();
				
				inStream.close();
				inStream = null;
				outStream.close();
				outStream = null;
				
				// second file
				inPathSB.delete(inBugDirLen, inPathSB.length());
				inPathSB.append(bugReportId);
				inPathSB.append("_stack.bin");
				File stackFile = new File(inPathSB.toString());
				
				notifyBugReportProgress(BugCatchOp.BugsToSDCard,
						mBugCatcher.getString(R.string.bugCopyingProgress),
						bugReportId, count, total);
				count++;
				
				if (!stackFile.exists()) {
					if (Logging.INFO) Log.i(TAG, "No stack for report bug "+bugReportId);
					continue; // skip if doesnt exists
				}
				
				if (Logging.DEBUG) Log.d(TAG, "Copying stack bugreport "+bugReportId);
				
				inStream = new FileInputStream(stackFile);
				
				outPathSB.delete(outBugDirLen, outPathSB.length());
				outPathSB.append(bugReportId);
				outPathSB.append("_stack.bin");
				outStream = new FileOutputStream(outPathSB.toString());
				
				while (true) {
					int readed = inStream.read(buffer);
					if (readed == -1)
						break;
					outStream.write(buffer, 0, readed);
				}
				
				// delay
				/*try {
					Thread.sleep(2000);
				} catch(InterruptedException ex) {
					notifyBugReportCancel(BugCatchOp.BugsToSDCard,
							mBugCatcher.getString(R.string.bugCopyingCancel));
					return;
				}*/
				
				if (Thread.interrupted()) {
					notifyBugReportCancel(BugCatchOp.BugsToSDCard,
							mBugCatcher.getString(R.string.bugCopyingCancel));
					return;
				}
				outStream.flush();
				
				inStream.close();
				inStream = null;
				outStream.close();
				outStream = null;
			}
			
			notifyBugReportFinish(BugCatchOp.BugsToSDCard,
					mBugCatcher.getString(R.string.bugCopyingFinished));
		} catch(Exception ex) {
			// end if error
			if (Logging.WARNING) Log.w(TAG, "Cant copy bugReportId");
			notifyBugReportError(BugCatchOp.BugsToSDCard,
					mBugCatcher.getString(R.string.bugCopyingError), bugReportId);
			notifyWorking(false);
			return;
		} finally {
			try {
				if (inStream != null)
					inStream.close();
			} catch(IOException ex) { }
			try {
				if (outStream != null)
					outStream.close();
			} catch(IOException ex) { }
			
			notifyWorking(false);
			Thread.interrupted(); // clear flag
		}
	}
	
	private static final int NOTIFY_PERIOD = 400;
	
	public void sendBugsToAuthor() {
		notifyWorking(true);
		
		File bugCatchDir = new File(mBugCatcher.getFilesDir().getAbsolutePath()+"/bugcatch");
		
		if (!bugCatchDir.exists()) {
			if (Logging.INFO) Log.i(TAG, "BugCatcher directory doesnt exists");
			notifyBugReportFinish(BugCatchOp.BugsToSDCard,
					mBugCatcher.getString(R.string.bugCopyingFinished));
			notifyWorking(false);
			return;
		}
		
		StringBuilder inPathSB = new StringBuilder();
		inPathSB.append(bugCatchDir.getAbsolutePath());
		inPathSB.append('/');
		int inBugDirLen = inPathSB.length();
		
		notifyBugReportBegin(BugCatchOp.SendBugs, mBugCatcher.getString(R.string.bugSendingBegin));
		
		byte[] buffer = new byte[1024];
		BugCatcherService.openLockForRead();
		File[] inFiles = bugCatchDir.listFiles();
		BugCatcherService.closeLockForRead();
		
		List<File> filteredFiles = filterBugReportContentFiles(inFiles);
		int count = 0;
		int total = filteredFiles.size();
		int progressCount;
		int progressTotal = total*200;
		
		long bugReportId = 0;
		
		FileInputStream inStream = null;
		DataOutputStream outStream = null;
		BufferedReader connInReader = null;
		HttpURLConnection conn = null;
		
		String bugReportUrl = mBugCatcher.getString(R.string.bugReportUrl);
		
		try {
			for (File file: filteredFiles) {
				String fname = file.getName();
				String idString = null;
				if (fname.endsWith("_content.txt"))
					idString = fname.substring(0, fname.length()-12);
				
				try {
					bugReportId = Long.parseLong(idString);
				} catch(ParseException ex) {
					if (Logging.WARNING) Log.w(TAG, "Cant parse bugReportId");
					count++;
					continue;
				}
				
				if (Logging.DEBUG) Log.d(TAG, "Sending bugreport "+bugReportId);
				
				progressCount = count*200;
				notifyBugReportProgress(BugCatchOp.SendBugs,
						mBugCatcher.getString(R.string.bugSendingProgress),
						bugReportId, progressCount, progressTotal);
				
				if (Thread.interrupted()) {
					notifyBugReportCancel(BugCatchOp.SendBugs,
							mBugCatcher.getString(R.string.bugSendingCancel));
					return;
				}
				
				/* checking content */
				{
					boolean isBroken = false;
					BufferedReader reader = null;
					try {
						reader = new BufferedReader(new FileReader(file));
						String header = reader.readLine(); // skip first line
						reader.readLine(); // skip second line
						String content = reader.readLine(); // content
						
						if (content == null || !content.startsWith("CommandLine:") ||
								header == null ||
								!header.equals("---- NATIVEBOINC BUGCATCH REPORT HEADER ----"))
							isBroken = true;
					} catch(IOException ex) {
						isBroken = true;
					} finally {
						if (reader != null)
							reader.close();
					}
					
					if (isBroken) {
						if (Logging.WARNING) Log.w(TAG, "File "+bugReportId+" is BROKEN!");
						// second file
						inPathSB.delete(inBugDirLen, inPathSB.length());
						inPathSB.append(bugReportId);
						inPathSB.append("_stack.bin");
						File stackFile = new File(inPathSB.toString());
						
						BugCatcherService.openLockForWrite();
						file.delete();
						if (stackFile.exists())
							stackFile.delete();
						BugCatcherService.closeLockForWrite();
						count++;
						continue;
					}
				}
				
				/*** create POST request ***/
				URL url = new URL(bugReportUrl);
				conn = (HttpURLConnection)url.openConnection();
				conn.setDoInput(true);
				conn.setDoOutput(true);
				conn.setUseCaches(false);
				conn.setRequestMethod("POST");
				conn.setRequestProperty("Connection", "Keep-Alive");
				conn.setRequestProperty("Content-Type", "multipart/form-data;boundary=cdHR5fWD8kWSa1Xa");
				
				outStream = new DataOutputStream(conn.getOutputStream());
				inStream = new FileInputStream(file);
				
				long fileLength = file.length();
				
				outStream.writeBytes("--cdHR5fWD8kWSa1Xa\r\n");
				outStream.writeBytes("Content-Disposition: form-data; name=\"content\";filename=\"content\"\r\n");
				outStream.writeBytes("Content-Type: text/plain;charset=UTF-8\r\n");
				outStream.writeBytes("Content-Length: "+fileLength+"\r\n");
				outStream.writeBytes("Content-Transfer-Encoding: binary\r\n");
				outStream.writeBytes("\r\n");
				
				long processed = 0;
				long currentTime = System.currentTimeMillis();
				
				while (true) {
					int readed = inStream.read(buffer);
					if (readed == -1)
						break;
					outStream.write(buffer, 0, readed);
					processed += readed;
					
					long newTime = System.currentTimeMillis();
					if (newTime - currentTime >= NOTIFY_PERIOD) {
						int addend = (int)((processed*100)/fileLength);
						notifyBugReportProgress(BugCatchOp.SendBugs,
								mBugCatcher.getString(R.string.bugSendingProgress),
								bugReportId, progressCount+addend, progressTotal);
						currentTime = newTime;
					}
				}
				
				inStream.close();
				inStream = null;
				
				// second file
				inPathSB.delete(inBugDirLen, inPathSB.length());
				inPathSB.append(bugReportId);
				inPathSB.append("_stack.bin");
				File stackFile = new File(inPathSB.toString());
				if (stackFile.exists()) { // if exists
					outStream.writeBytes("\r\n--cdHR5fWD8kWSa1Xa\r\n");
					
					if (Thread.interrupted()) {
						notifyBugReportCancel(BugCatchOp.SendBugs,
								mBugCatcher.getString(R.string.bugSendingCancel));
						return;
					}
					
					if (Logging.DEBUG) Log.d(TAG, "Sending stack bugreport "+bugReportId);
					
					progressCount = count*200+100;
					notifyBugReportProgress(BugCatchOp.SendBugs,
							mBugCatcher.getString(R.string.bugSendingProgress),
							bugReportId, progressCount, progressTotal);
					
					outStream.writeBytes("Content-Disposition: form-data; name=\"stack\";filename=\"stack\"\r\n");
					outStream.writeBytes("Content-Type: application/octet-stream\r\n");
					outStream.writeBytes("Content-Length: "+stackFile.length()+"\r\n");
					outStream.writeBytes("Content-Transfer-Encoding: binary\r\n");
					outStream.writeBytes("\r\n");
					
					inStream = new FileInputStream(stackFile);
					
					currentTime = System.currentTimeMillis();
					while (true) {
						int readed = inStream.read(buffer);
						if (readed == -1)
							break;
						outStream.write(buffer, 0, readed);
						
						long newTime = System.currentTimeMillis();
						if (newTime - currentTime >= NOTIFY_PERIOD) {
							int addend = (int)((processed*100)/fileLength);
							notifyBugReportProgress(BugCatchOp.SendBugs,
									mBugCatcher.getString(R.string.bugSendingProgress),
									bugReportId, progressCount+addend, progressTotal);
							currentTime = newTime;
						}
					}
					
					inStream.close();
					inStream = null;
					
					outStream.writeBytes("\r\n--cdHR5fWD8kWSa1Xa--\r\n");
				} else {
					outStream.writeBytes("\r\n--cdHR5fWD8kWSa1Xa--\r\n");
					if (Logging.INFO) Log.i(TAG, "No stack for report bug "+bugReportId);
				}
				
				outStream.flush();
				
				/* read response */
				connInReader = new BufferedReader(new InputStreamReader(conn.getInputStream(), "UTF-8"));
				
				if (conn.getResponseCode() != 200) {
					notifyBugReportError(BugCatchOp.SendBugs,
							mBugCatcher.getString(R.string.bugSendingError), bugReportId);
					return;
				}
				
				String s = connInReader.readLine();
				if (s == null || !s.contains("\"OK\"")) {
					notifyBugReportError(BugCatchOp.SendBugs,
							mBugCatcher.getString(R.string.bugSendingError), bugReportId);
					return;
				}
				
				connInReader.close();
				connInReader = null;
				outStream.close();
				outStream = null;
				
				conn.disconnect();
				conn = null;
				
				// deleting obsolete bugreport
				BugCatcherService.openLockForWrite();
				file.delete();
				if (stackFile.exists())
					stackFile.delete();
				BugCatcherService.closeLockForWrite();
				
				count++;
			}
			
			notifyBugReportFinish(BugCatchOp.SendBugs,
					mBugCatcher.getString(R.string.bugSendingFinished));
			
		} catch(InterruptedIOException ex) {
			notifyBugReportCancel(BugCatchOp.SendBugs,
					mBugCatcher.getString(R.string.bugSendingCancel));
			return; // cancelled
		} catch(Exception ex) {
			notifyBugReportError(BugCatchOp.SendBugs,
					mBugCatcher.getString(R.string.bugSendingError), bugReportId);
			return;
		} finally {
			try {
				if (inStream != null)
					inStream.close();
			} catch(IOException ex) { }
			
			try {
				if (outStream != null)
					outStream.close();
			} catch(IOException ex) { }
			
			try {
				if (connInReader != null)
					connInReader.close();
			} catch(IOException ex) { }
			
			if (conn != null)
				conn.disconnect();
			
			notifyWorking(false);
			Thread.interrupted(); // clear flag
		}
	}
	
	/**
	 * notifying routines via listener handler
	 */
	private synchronized void notifyBugReportBegin(final BugCatchOp bugCatchOp, final String desc) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onBugReportBegin(bugCatchOp, desc);
			}
		});
	}
	
	private synchronized void notifyBugReportProgress(final BugCatchOp bugCatchOp, final String desc,
			final long bugReportId, final int count, final int total) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onBugReportProgress(bugCatchOp, desc, bugReportId, count, total);
			}
		});
	}
	
	private synchronized void notifyBugReportError(final BugCatchOp bugCatchOp, final String desc,
			final long bugReportId) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onBugReportError(bugCatchOp, desc, bugReportId);
			}
		});
	}
	
	private synchronized void notifyBugReportCancel(final BugCatchOp bugCatchOp, final String desc) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onBugReportCancel(bugCatchOp, desc);
			}
		});
	}
	
	private synchronized void notifyBugReportFinish(final BugCatchOp bugCatchOp, final String desc) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onBugReportFinish(bugCatchOp, desc);
			}
		});
	}
	
	private synchronized void notifyWorking(final boolean isWorking) {
		boolean prevIsWorking = mIsWorking;
		if (prevIsWorking != isWorking) {
			mIsWorking = isWorking;
			mListenerHandler.post(new Runnable() {
				@Override
				public void run() {
					mListenerHandler.onChangeIsWorking(isWorking);
				}
			});
		}
		if (!isWorking && mBugCatcher.doStopWhenNotWorking()) {
			// stop service
			Log.d(TAG, "Stop when not working");
			mBugCatcher.stopSelf();
		}
	}
}
