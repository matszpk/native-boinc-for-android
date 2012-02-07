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
import java.io.InputStream;
import java.io.InterruptedIOException;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Map;
import java.util.ArrayList;
import java.util.concurrent.Semaphore;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import edu.berkeley.boinc.nativeboinc.ClientEvent;

import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.MonitorListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
import sk.boinc.nativeboinc.nativeclient.NativeBoincStateListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincUpdateListener;
import sk.boinc.nativeboinc.util.Chmod;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.HostListDbAdapter;
import sk.boinc.nativeboinc.util.PreferenceName;
import sk.boinc.nativeboinc.util.UpdateItem;
import sk.boinc.nativeboinc.util.VersionUtil;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.util.Log;

/**
 * @author mat
 *
 */
public class InstallerHandler extends Handler implements NativeBoincUpdateListener,
	NativeBoincStateListener, MonitorListener {
	
	private final static String TAG = "InstallerHandler";

	private Context mContext = null;
	
	private InstallerService.ListenerHandler mListenerHandler;
	
	private InstalledDistribManager mDistribManager = null;
	private ArrayList<ProjectDistrib> mProjectDistribs = null;
	private ClientDistrib mClientDistrib = null;
	
	private static final int NOTIFY_PERIOD = 200; /* milliseconds */
	
	private boolean mIsGloballyCancelled = false;
	private boolean mClientInstallCancelled = false;
	
	private NativeBoincService mRunner = null;
	
	private boolean mIsClientBeingInstalled = false;
	private Semaphore mDelayedClientShutdownSem = new Semaphore(1);
	
	private Downloader mDownloader = null;
	
	/*
	 * cancel observers - can talk whether operation is cancelled
	 */
	
	private class GlobalCancelObserver implements CancelObserver {
		public boolean isCancelled() {
			synchronized(InstallerHandler.this) {
				return mIsGloballyCancelled;
			}
		}
	}
	
	private GlobalCancelObserver mGlobalCancelObserver = new GlobalCancelObserver();
	
	private class ClientInstallCancelObserver implements CancelObserver {
		public boolean isCancelled() {
			synchronized(InstallerHandler.this) {
				return mIsGloballyCancelled || mClientInstallCancelled;
			}
		}
	}
	
	private ClientInstallCancelObserver mClientInstallCancelObserver =
			new ClientInstallCancelObserver();
	
	private class AppInstallCancelObserver implements CancelObserver {
		private ProjectAppsInstaller mAppInstaller;
		
		public AppInstallCancelObserver(ProjectAppsInstaller appInstaller) {
			mAppInstaller = appInstaller;
		}
		
		public boolean isCancelled() {
			synchronized(InstallerHandler.this) {
				return mIsGloballyCancelled || mAppInstaller.isCancelled();
			}
		}
	}
	
	public InstallerHandler(final Context context,
			InstallerService.ListenerHandler listenerHandler) {
		mContext = context;
		mDownloader = new Downloader(context, listenerHandler);
		mListenerHandler = listenerHandler;
		mDistribManager = new InstalledDistribManager(context);
		mDistribManager.load();
	}
	
	private synchronized void resetGlobalCanceller() {
		mIsGloballyCancelled = false;
	}
	
	private synchronized void resetAllCancellers() {
		mIsGloballyCancelled = false;
		mClientInstallCancelled = false;
	}
	
	public void setRunnerService(NativeBoincService service) {
		mRunner = service;
	}
	
	public void updateClientDistrib() {
		resetGlobalCanceller();
		
		String clientListUrl = mContext.getString(R.string.installClientSourceUrl)+"client.xml";
		
		if (mGlobalCancelObserver.isCancelled()) {
			notifyCancel("", "");
			return;
		}
		
		mClientDistrib = null;
		
		/* download boinc client's list */
		try {	/* download with ignoring */
			mDownloader.downloadFile(clientListUrl, "client.xml",
					mContext.getString(R.string.clientListDownload), 
					mContext.getString(R.string.clientListDownloadError), false,
					"", "", mGlobalCancelObserver);
			
			if (mGlobalCancelObserver.isCancelled()) {
				mContext.deleteFile("client.xml");
				notifyCancel("", "");
				return;
			}
			
			/* if doesnt downloaded */
			if (!mContext.getFileStreamPath("client.xml").exists()) {
				notifyClientDistrib(null);
				return;
			}
			
			int status = mDownloader.verifyFile(mContext.getFileStreamPath("client.xml"),
					clientListUrl, false, "", "", mGlobalCancelObserver);
			
			if (status == Downloader.VERIFICATION_CANCELLED) {
				mContext.deleteFile("client.xml");
				notifyCancel("", "");
				return;	// cancelled
			}
			if (status == Downloader.VERIFICATION_FAILED)
				notifyError("", "", mContext.getString(R.string.verifySignatureFailed));
		} catch(InstallationException ex) {
			mContext.deleteFile("client.xml");
			return;
		}
		
		/* parse it */
		InputStream inStream = null;
		
		mClientDistrib = null;
		try {
			inStream = mContext.openFileInput("client.xml");
			/* parse and notify */
			mClientDistrib = ClientDistribListParser.parse(inStream);
			if (mClientDistrib == null)
				notifyError("", "", mContext.getString(R.string.clientListParseError));
		} catch(IOException ex) {
			notifyError("", "", mContext.getString(R.string.clientListParseError));
			return;
		} finally {
			try {
				inStream.close();
			} catch(IOException ex) { }
			
			/* delete obsolete file */
			mContext.deleteFile("client.xml");
		}
		
		notifyClientDistrib(mClientDistrib);
	}
	
	private static final int BUFFER_SIZE = 4096;
	
	public void installClientAutomatically(boolean standalone) {
		if (mIsClientBeingInstalled) {
			notifyFinish(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
			return;	// skip
		}
		
		mIsClientBeingInstalled = true;
		// if not concurrent thread set as current
		if (mClientInstallerThread == null) {
			mClientInstallerThread = Thread.currentThread();
		}
		
		boolean clientToUpdate = isClientInstalled();
		
		if (standalone)	{// if standalone called from InstallerService
			if (Logging.DEBUG) Log.d(TAG, "Install client standalone");
			resetAllCancellers();
		}
		
		boolean clientIsRan = mRunner.isRun();
		String boincClientFilename = "boinc_client";
		if (clientIsRan) {
			if (Logging.DEBUG) Log.d(TAG, "Install client when ran.");
			try {
				mDelayedClientShutdownSem.acquire();
			} catch(InterruptedException ex) { }
			mRunner.shutdownClient();
			boincClientFilename = "boinc_client_new";
		}
		
		try {
			String zipFilename = null;
			
			if (mClientInstallCancelObserver.isCancelled()) {
				notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
				return;
			}
			
			if (mClientDistrib == null) {
				notifyError(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
						mContext.getString(R.string.clientDistribNotFound));
				return;
			}
		
			
			zipFilename = mClientDistrib.filename;
			if (Logging.INFO) Log.i(TAG, "Use zip "+zipFilename);
			
			if (mClientInstallCancelObserver.isCancelled()) {
				notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
				return;
			}
			
			/* download and unpack */
			try {
				String zipUrlString = mContext.getString(R.string.installClientSourceUrl)+zipFilename; 
				mDownloader.downloadFile(zipUrlString, "boinc_client.zip",
						mContext.getString(R.string.downloadNativeClient),
						mContext.getString(R.string.downloadNativeClientError), true,
						InstallerService.BOINC_CLIENT_ITEM_NAME, "", mClientInstallCancelObserver);
				
				if (mClientInstallCancelObserver.isCancelled()) {
					mContext.deleteFile("boinc_client.zip");
					notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
					return;
				}
				
				int status = mDownloader.verifyFile(mContext.getFileStreamPath("boinc_client.zip"),
						zipUrlString, true, InstallerService.BOINC_CLIENT_ITEM_NAME, "",
						mClientInstallCancelObserver);
				
				if (status == Downloader.VERIFICATION_CANCELLED) {
					mContext.deleteFile("boinc_client.zip");
					notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
					return;	// cancelled
				}
				if (status == Downloader.VERIFICATION_FAILED)
					notifyError(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
							mContext.getString(R.string.verifySignatureFailed));
			} catch(InstallationException ex) {
				/* remove zip file */
				mContext.deleteFile("boinc_client.zip");
				return;
			}
			
			if (mClientInstallCancelObserver.isCancelled()) {
				mContext.deleteFile("boinc_client.zip");
				notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
				return;
			}
			
			FileOutputStream outStream = null;
			InputStream zipStream = null;
			ZipFile zipFile = null;
			/* unpack zip file */
			try {
				zipFile = new ZipFile(mContext.getFileStreamPath("boinc_client.zip"));
				ZipEntry zipEntry = zipFile.entries().nextElement();
				zipStream = zipFile.getInputStream(zipEntry);
				outStream = mContext.openFileOutput(boincClientFilename, Context.MODE_PRIVATE);
				
				long time = System.currentTimeMillis();
				long length = zipEntry.getSize();
				int totalReaded = 0;
				byte[] buffer = new byte[BUFFER_SIZE];
				/* copying content to file */
				String opDesc = mContext.getString(R.string.unpackNativeClient);
				while (true) {
					int readed = zipStream.read(buffer);
					if (readed == -1)
						break;
					totalReaded += readed;
					
					if (mClientInstallCancelObserver.isCancelled()) {
						mContext.deleteFile(boincClientFilename);
						mContext.deleteFile("boinc_client.zip");
						notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
						return;
					}
					
					outStream.write(buffer, 0, readed);
					long newTime = System.currentTimeMillis();
					
					if (newTime-time > NOTIFY_PERIOD) {
						notifyProgress(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
								opDesc, (int)((double)totalReaded*10000.0/(double)length));
						time = newTime;
					}
				}
				notifyProgress(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
						opDesc, InstallerProgressListener.FINISH_PROGRESS);
			} catch(InterruptedIOException ex) {
				notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
				mContext.deleteFile(boincClientFilename);
				return;
			} catch(IOException ex) {
				notifyError(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
						mContext.getString(R.string.unpackNativeClientError));
				mContext.deleteFile(boincClientFilename);
				return;
			} finally {
				try {
					if (zipStream != null)
						zipStream.close();
				} catch(IOException ex) { }
				try {
					if (zipFile != null)
						zipFile.close();
				} catch(IOException ex) { }
				try {
					if (outStream != null)
						outStream.close();
				} catch(IOException ex) { }
				/* remove zip file */
				mContext.deleteFile("boinc_client.zip");
			}
			
			if (mClientInstallCancelObserver.isCancelled()) {
				mContext.deleteFile("boinc_client.zip");
				mContext.deleteFile(boincClientFilename);
				notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
				return;
			}
			
			/* change permissions */
			if (!Chmod.chmod(mContext.getFileStreamPath(boincClientFilename).getAbsolutePath(), 0700)) {
				notifyError(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
						mContext.getString(R.string.changePermissionsError));
		    	mContext.deleteFile(boincClientFilename);
		    	return;
			}
			
		    if (mClientInstallCancelObserver.isCancelled()) {
				notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
				return;
			}
		    
		    /* create boinc directory */
		    File boincDir = new File(mContext.getFilesDir()+"/boinc");
		    if (!boincDir.isDirectory()) {
		    	boincDir.delete();
		    	boincDir.mkdir();
		    }
		    
		    if (mClientDistrib != null) {
		    	mDistribManager.setClient(mClientDistrib);
		    	mDistribManager.save();
		    }
		    
		    /* running and killing boinc client (run in this client) */
		    if (!clientToUpdate) {
		    	if (Logging.DEBUG) Log.d(TAG, "First start client");
		    	
		    	notifyOperation(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
		    			mContext.getString(R.string.nativeClientFirstStart));
		    	
			    if (!mRunner.firstStartClient()) {
			    	notifyError(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
			    			mContext.getString(R.string.nativeClientFirstStartError));
			    	return;
			    }
			    notifyOperation(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
		    			mContext.getString(R.string.nativeClientFirstKill));	    	
		    }
		    
		    if (clientIsRan) {
		    	if (Logging.DEBUG) Log.d(TAG, "Rename to real boinc");
		    	try {
			    	mDelayedClientShutdownSem.acquire();
			    	// rename new boinc client file
			    	mContext.getFileStreamPath("boinc_client_new").renameTo(
			    			mContext.getFileStreamPath("boinc_client"));
			    	/* give control over installation to NativeBoincService (second start) */
			    	mIsClientBeingInstalled = false;
			    	mRunner.startClient(true);
			    	mDelayedClientShutdownSem.release();
		    	} catch(InterruptedException ex) { }
		    } else {
		    	// add nativeboinc to client list
		    	HostListDbAdapter dbAdapter = null;
		    	
		    	if (Logging.DEBUG) Log.d(TAG, "Adding nativeboinc to hostlist");
		    	
		    	try {
		    		String accessPassword = mRunner.getAccessPassword();
		    		dbAdapter = new HostListDbAdapter(mContext);
		    		dbAdapter.open();
		    		dbAdapter.addHost(new ClientId(0, "nativeboinc", "127.0.0.1",
		    				31416, accessPassword));		    		
		    	} catch(IOException ex) {
		    		notifyError(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
		    				mContext.getString(R.string.getAccessPasswordError));
		    	} finally {
		    		if (dbAdapter != null)
		    			dbAdapter.close();
		    	}
		    	
		    	// starting client
		    	mRunner.startClient(true);
		    	mIsClientBeingInstalled = false;
		    }
		    
		    notifyFinish(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
		} finally {
			if (Logging.DEBUG) Log.d(TAG, "Release semaphore mDelayedClientShutdownSem");
			mDelayedClientShutdownSem.release();
			mIsClientBeingInstalled = false;
		}
	}
	
	/* from native boinc lib */
	public static String escapeProjectUrl(String url) {
		StringBuilder out = new StringBuilder();
		
		int i = url.indexOf("://");
		if (i == -1)
			i = 0;
		else
			i += 3;
		
		for (; i < url.length(); i++) {
			char c = url.charAt(i);
			if (Character.isLetterOrDigit(c) || c == '.' || c == '_' || c == '-')
				out.append(c);
			else
				out.append('_');
		}
		/* remove trailing '_' */
		if (out.length() >= 1 && out.charAt(out.length()-1) == '_')
			out.deleteCharAt(out.length()-1);
		return out.toString();
	}
	
	private ArrayList<String> unpackProjectApplications(ProjectDistrib projectDistrib, String zipPath,
			final CancelObserver cancelObserver) {
		/* */
		if (cancelObserver.isCancelled()) {
			return null;
		}
		
		ArrayList<String> fileList = new ArrayList<String>();
		FileOutputStream outStream = null;
		ZipFile zipFile = null;
		InputStream inStream = null;
		
		long totalLength = 0;
		
		StringBuilder projectAppFilePath = new StringBuilder();
		projectAppFilePath.append(mContext.getFilesDir());
		projectAppFilePath.append("/boinc/updates/");
		projectAppFilePath.append(escapeProjectUrl(projectDistrib.projectUrl));
		projectAppFilePath.append("/");
		
		File updateDir = new File(projectAppFilePath.toString());
		
		if (!updateDir.isDirectory())
			updateDir.mkdirs();
		
		int projectDirPathLength = projectAppFilePath.length();
		
		try {
			zipFile = new ZipFile(zipPath);
			Enumeration<? extends ZipEntry> zipEntries = zipFile.entries();
			
			/* count total length of uncompressed files */
			while (zipEntries.hasMoreElements()) {
				ZipEntry entry = zipEntries.nextElement();
				if (!entry.isDirectory()) {
					totalLength += entry.getSize();

					String entryName = entry.getName();
					int slashIndex = entryName.lastIndexOf('/');
					if (slashIndex != -1)
						entryName = entryName.substring(slashIndex+1);
					
					fileList.add(entryName);
				}
			}
			
			long totalReaded = 0;
			byte[] buffer = new byte[BUFFER_SIZE];
			/* unpack zip */
			zipEntries = zipFile.entries();
			
			int i = 0;
			long time = System.currentTimeMillis();
			
			String opDesc = mContext.getString(R.string.unpackApplication);
			
			while (zipEntries.hasMoreElements()) {
				ZipEntry entry = zipEntries.nextElement();
				if (entry.isDirectory())
					continue;
				
				inStream = zipFile.getInputStream(entry);
				
				projectAppFilePath.append(fileList.get(i));
				outStream = new FileOutputStream(projectAppFilePath.toString());
				
				/* copying to project file */
				while (true) {
					int readed = inStream.read(buffer);
					if (readed == -1)
						break;
					
					totalReaded += readed;
					
					if (cancelObserver.isCancelled()) {
						notifyCancel(projectDistrib.projectName, projectDistrib.projectUrl);
						return null;
					}
					
					outStream.write(buffer, 0, readed);
					
					long newTime = System.currentTimeMillis();
					if (newTime-time > NOTIFY_PERIOD) {
						notifyProgress(projectDistrib.projectName, projectDistrib.projectUrl,
								opDesc, (int)((double)totalReaded*10000.0/(double)totalLength));
						time = newTime;
					}
				}
				
				/* reverts to project dir path */
				projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
				outStream = null;
				i++;
			}
			
		} catch(InterruptedIOException ex) {
			projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
			notifyCancel(projectDistrib.projectName, projectDistrib.projectUrl);
			return null;
		} catch(IOException ex) {
			projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
			
			notifyError(projectDistrib.projectName, projectDistrib.projectUrl,
					mContext.getString(R.string.unpackApplicationError));
			/* delete files */
			if (fileList != null) {
				for (String filename: fileList) {
					projectAppFilePath.append(filename);
					new File(projectAppFilePath.toString()).delete();
					projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
				}
			}
			return null;
		} finally {
			try {
				if (zipFile != null)
					zipFile.close();
			} catch(IOException ex2) { }
			try {
				if (inStream != null)
					inStream.close();
			} catch(IOException ex2) { }
			try {
				if (outStream != null)
					outStream.close();
			} catch(IOException ex2) { }
		}
		
		if (cancelObserver.isCancelled()) {
			notifyCancel(projectDistrib.projectName, projectDistrib.projectUrl);
			return null;
		}
		
		/* change permissions */
		ArrayList<String> execFilenames = null;
		try {
			inStream = null;
			projectAppFilePath.append("app_info.xml");
			inStream = new FileInputStream(projectAppFilePath.toString());
			execFilenames = ExecFilesAppInfoParser.parse(inStream);
		} catch(IOException ex) {
		} finally {
			projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
			try {
				if (inStream != null)
					inStream.close();
			} catch(IOException ex) { }
		}
		
		if (cancelObserver.isCancelled()) {
			notifyCancel(projectDistrib.projectName, projectDistrib.projectUrl);
			return null;
		}
		
		if (execFilenames != null) {
			for (String filename: execFilenames) {
				projectAppFilePath.append(filename);
				if (!Chmod.chmod(projectAppFilePath.toString(), 0700)) {
					notifyError(projectDistrib.projectName, projectDistrib.projectUrl,
							mContext.getString(R.string.changePermissionsError));
			    	return null;
				}
				projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
			}
		}
		
		if (cancelObserver.isCancelled()) {
			notifyCancel(projectDistrib.projectName, projectDistrib.projectUrl);
			return null;
		}
		
		return fileList;
	}
	
	private class ProjectAppsInstaller extends Thread implements CancelObserver {
		private boolean mIsCancelled;
		private final CancelObserver mCancelObserver;
		private String mZipFilename;
		private ProjectDistrib mProjectDistrib;
		private ArrayList<String> mFileList;
		
		/* if client not installed and ran then acquired */ 
		private Semaphore mClientUpdatedAndRanSem = new Semaphore(1);
		
		/* should be released after benchmark */
		private Semaphore mBenchmarkFinishSem = new Semaphore(1);
		
		public ProjectAppsInstaller(String zipFilename, ProjectDistrib projectDistrib) {
			mZipFilename = zipFilename;
			mProjectDistrib = projectDistrib;
			mCancelObserver = new AppInstallCancelObserver(this);
			/* acquiring all semaphores */
			try {
				mClientUpdatedAndRanSem.acquire();
			} catch(InterruptedException ex) { }
			try {
				mBenchmarkFinishSem.acquire();
			} catch(InterruptedException ex) { }
			mIsCancelled = false;
		}
		
		@Override
		public void run() {
			if (mCancelObserver.isCancelled()) {
				notifyCancel(mProjectDistrib.projectName, mProjectDistrib.projectUrl);
				return;
			}
			
			/* awaiting for benchmark finish */
			notifyOperation(mProjectDistrib.projectName, mProjectDistrib.projectUrl,
					mContext.getString(R.string.waitingForBenchmarkFinish));
			try {
				mBenchmarkFinishSem.acquire();
			} catch(InterruptedException ex) {
				Log.d(TAG, "Cancelled during waiting benchmark finish");
				notifyCancel(mProjectDistrib.projectName, mProjectDistrib.projectUrl);
				return;
			}
			mBenchmarkFinishSem.release();
			
			String zipUrl = mContext.getString(R.string.installAppsSourceUrl) + mZipFilename;
			
			/* do download and verify */
			String outZipFilename = mProjectDistrib.projectName+".zip";
			try {
				mDownloader.downloadFile(zipUrl, outZipFilename,
						mContext.getString(R.string.downloadApplication),
						mContext.getString(R.string.downloadApplicationError), true,
						mProjectDistrib.projectName, mProjectDistrib.projectUrl, mCancelObserver);
				
				if (mCancelObserver.isCancelled()) {
					mContext.deleteFile(outZipFilename);
					notifyCancel(mProjectDistrib.projectName, mProjectDistrib.projectUrl);
					return;
				}
				
				int status = mDownloader.verifyFile(mContext.getFileStreamPath(outZipFilename),
						zipUrl, true, mProjectDistrib.projectName, mProjectDistrib.projectUrl,
						mCancelObserver);
				
				if (status == Downloader.VERIFICATION_CANCELLED) {
					mContext.deleteFile(outZipFilename);
					notifyCancel(mProjectDistrib.projectName, mProjectDistrib.projectUrl);
					return;	// cancelled
				}
				if (status == Downloader.VERIFICATION_FAILED)
					notifyError(mProjectDistrib.projectName, mProjectDistrib.projectUrl,
							mContext.getString(R.string.verifySignatureFailed));
			} catch(InstallationException ex) {
				mContext.deleteFile(outZipFilename);
				return;
			}
			
			if (mCancelObserver.isCancelled()) {
				mContext.deleteFile(outZipFilename);
				notifyCancel(mProjectDistrib.projectName, mProjectDistrib.projectUrl);
				return;
			}
			
			/* do install in project directory */
			mFileList = unpackProjectApplications(mProjectDistrib,
					mContext.getFileStreamPath(outZipFilename).getAbsolutePath(), mCancelObserver);
			
			mContext.deleteFile(outZipFilename);
			
			if (mCancelObserver.isCancelled()) {
				notifyCancel(mProjectDistrib.projectName, mProjectDistrib.projectUrl);
				return;
			}
			
			notifyOperation(mProjectDistrib.projectName, mProjectDistrib.projectUrl,
					mContext.getString(R.string.waitingForClientUpdateAndRun));
			
			try {
				mClientUpdatedAndRanSem.acquire();
			} catch(InterruptedException ex) {
				Log.d(TAG, "Cancelled during waiting for client update");
				notifyCancel(mProjectDistrib.projectName, mProjectDistrib.projectUrl);
				return;
			}
			mClientUpdatedAndRanSem.release();
			
			notifyOperation(mProjectDistrib.projectName, mProjectDistrib.projectUrl,
					mContext.getString(R.string.finalizeProjectAppInstall));
			
			/* update in client side */
			if (Logging.DEBUG) Log.d(TAG, "Run update_apps on boinc_client for "+
						mProjectDistrib.projectName);
			mRunner.updateProjectApps(mProjectDistrib.projectUrl);
		}
		
		/**
		 * used by AppInstallCancelObserver
		 */
		public boolean isCancelled() {
			return mIsCancelled;
		}
		
		public void cancelOperation() {
			mIsCancelled = true;
			interrupt();
		}
		
		public synchronized void notifyIfClientUpdatedAndRan() {
			mClientUpdatedAndRanSem.release();
		}
		
		public synchronized void notifyIfBenchmarkFinished() {
			mBenchmarkFinishSem.release();
		}
	}
	
	/* this map contains  app installer threads, key - projectUrl, value - app installer */
	private Map<String, ProjectAppsInstaller> mProjectAppsInstallers =
			new HashMap<String, ProjectAppsInstaller>();
	
	private synchronized void notifyIfClientUpdatedAndRan() {
		for (ProjectAppsInstaller appInstaller: mProjectAppsInstallers.values())
			appInstaller.notifyIfClientUpdatedAndRan();
	}
	
	private synchronized void notifyIfBenchmarkFinished() {
		for (ProjectAppsInstaller appInstaller: mProjectAppsInstallers.values())
			appInstaller.notifyIfBenchmarkFinished();
	}
	
	private synchronized boolean isProjectAppBeingInstalled(String projectUrl) {
		return mProjectAppsInstallers.containsKey(projectUrl);
	}
	
	private boolean mBenchmarkIsRun = false;
	
	private static final int MAX_PERIOD_BEFORE_BENCHMARK = 3000;
	
	public void installProjectApplicationsAutomatically(String projectUrl) {
		if (Logging.DEBUG) Log.d(TAG, "install App "+projectUrl);
		
		ProjectDistrib projectDistrib = null;
		
		String zipFilename = null;
		/* search project, and retrieve project url */
		for (ProjectDistrib distrib: mProjectDistribs)
			if (distrib.projectUrl.equals(projectUrl)) {
				zipFilename = distrib.filename;
				projectDistrib = distrib;
				break;
			}
		
		if (zipFilename == null) {
			notifyError(projectDistrib.projectName, projectUrl,
					mContext.getString(R.string.projectAppNotFound));
			return;
		}
		
		if (isProjectAppBeingInstalled(projectUrl)) {	// do nothing if installed
			notifyFinish(projectDistrib.projectName, projectUrl);
			return;
		}
		
		resetGlobalCanceller();
		
		// installBoincApplicationAutomatically(zipFilename, projectDistrib);
		// run in separate thread
		final ProjectAppsInstaller appInstaller = new ProjectAppsInstaller(zipFilename, projectDistrib);
		// unlock lock for client updating
		appInstaller.notifyIfClientUpdatedAndRan();
		
		postDelayed(new Runnable() {
			@Override
			public void run() {
				if (!mBenchmarkIsRun) // if again benchmark is not ran
					// unlock benchmark lock
					appInstaller.notifyIfBenchmarkFinished();
			}
		}, MAX_PERIOD_BEFORE_BENCHMARK);
		
		synchronized(this) {
			mProjectAppsInstallers.put(projectDistrib.projectUrl, appInstaller);
		}
		appInstaller.start();
	}
	
	private class ConcurrentClientInstaller extends Thread {
		@Override
		public void run() {
			installClientAutomatically(false);
			mClientInstallerThread = null;
		}
	}
	
	private Thread mClientInstallerThread = null;
	
	public void reinstallUpdateItems(ArrayList<UpdateItem> updateItems) {
		resetAllCancellers();
		
		boolean clientInstall = false;
		
		for (UpdateItem updateItem: updateItems) {
			if (updateItem.name.equals(InstallerService.BOINC_CLIENT_ITEM_NAME)) {
				/* update boinc client */
				clientInstall = true;
				mClientInstallerThread = new ConcurrentClientInstaller();
				mClientInstallerThread.start();
			} else {
				/* install */
				ProjectDistrib foundDistrib = null;
				for (ProjectDistrib distrib: mProjectDistribs)
					if (distrib.projectName.equals(updateItem.name)) {
						foundDistrib = distrib;
						break;
					}
				if (isProjectAppBeingInstalled(foundDistrib.projectUrl))
					continue;	// if being installed
				// run in separate thread
				ProjectAppsInstaller appInstaller =
						new ProjectAppsInstaller(updateItem.filename, foundDistrib);
				synchronized(this) {
					mProjectAppsInstallers.put(foundDistrib.projectUrl, appInstaller);
				}
				appInstaller.start();
			}
		}
		// unlocks project apps installer if client not installed
		if (!clientInstall) {
			if (Logging.DEBUG) Log.d(TAG, "if only project apps will being installed");
			notifyIfClientUpdatedAndRan();
		}
	}
	
	public void updateProjectDistribList() {
		mIsGloballyCancelled = false;
		
		String appListUrl = mContext.getString(R.string.installAppsSourceUrl)+"apps.xml";
		
		if (mGlobalCancelObserver.isCancelled()) {
			notifyCancel("", "");
			return;
		}
		
		try {
			mDownloader.downloadFile(appListUrl, "apps.xml",
					mContext.getString(R.string.appListDownload),
					mContext.getString(R.string.appListDownloadError), false, 
					"", "", mGlobalCancelObserver);
			
			if (mGlobalCancelObserver.isCancelled()) {
				mContext.deleteFile("apps.xml");
				notifyCancel("", "");
				return;
			}
			
			int status = mDownloader.verifyFile(mContext.getFileStreamPath("apps.xml"),
					appListUrl, false, "", "", mGlobalCancelObserver);
			
			if (status == Downloader.VERIFICATION_CANCELLED) {
				mContext.deleteFile("apps.xml");
				notifyCancel("", "");
				return;	// cancelled
			}
			if (status == Downloader.VERIFICATION_FAILED)
				notifyError("", "", mContext.getString(R.string.verifySignatureFailed));
		} catch(InstallationException ex) {
			mContext.deleteFile("apps.xml");
			return;
		}
		
		if (mGlobalCancelObserver.isCancelled()) {
			mContext.deleteFile("apps.xml");
			notifyCancel("", "");
			return;
		}
		
		/* parse it */
		InputStream inStream = null;
		try {
			inStream = mContext.openFileInput("apps.xml");
			/* parse and notify */
			mProjectDistribs = ProjectDistribListParser.parse(inStream);
			if (mProjectDistribs != null)
				notifyProjectDistribs(mProjectDistribs);
			else
				notifyError("", "", mContext.getString(R.string.appListParseError));
		} catch(IOException ex) {
			notifyError("", "", mContext.getString(R.string.appListParseError));
		} finally {
			try {
				inStream.close();
			} catch(IOException ex) { }
			
			/* delete obsolete file */
			mContext.deleteFile("apps.xml");
		}
	}
	
	/* remove detached projects from installed projects */
	public void synchronizeInstalledProjects() {
		mDistribManager.synchronizeWithProjectList(mContext);
	}
	
	private static final String[] sRequiredFiles = {
		"/boinc/client_state.xml",
		"/boinc/client_state_prev.xml",
		"/boinc/time_stats_log",
		"/boinc/lockfile",
		"/boinc/daily_xfer_history.xml",
		"/boinc/gui_rpc_auth.cfg"
	};
	
	public boolean isClientInstalled() {
		File filesDir = mContext.getFilesDir();
		if (!mContext.getFileStreamPath("boinc_client").exists() ||
			!mContext.getFileStreamPath("boinc").isDirectory())
			return false;
		File boincFile = new File(filesDir.getAbsolutePath()+"/boinc/gui_rpc_auth.cfg");
		if (!boincFile.exists())
			return false;
		for (String path: sRequiredFiles) {
			boincFile = new File(filesDir.getAbsolutePath()+path);
			if (!boincFile.exists())
				return false;	// if installation broken
		}
		return true;
	}
	
	public synchronized void cancelAll() {
		mIsGloballyCancelled = true;
		cancelClientInstallation();
		for (ProjectAppsInstaller appInstaller: mProjectAppsInstallers.values())
			appInstaller.cancelOperation();
		mProjectAppsInstallers.clear();
	}
	
	public synchronized void cancelClientInstallation() {
		mClientInstallCancelled = true;
		if (mClientInstallerThread != null)
			mClientInstallerThread.interrupt();
		mClientInstallerThread = null;
	}
	
	public synchronized boolean isWorking() {
		return  mIsClientBeingInstalled || mProjectAppsInstallers.isEmpty();
	}
	
	public synchronized void cancelProjectAppsInstallation(String projectUrl) {
		ProjectAppsInstaller appsInstaller = mProjectAppsInstallers.remove(projectUrl);
		if (appsInstaller != null)
			appsInstaller.cancelOperation();
	}
	
	public InstalledBinary[] getInstalledBinaries() {
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(mContext);
		String clientVersion = prefs.getString(PreferenceName.CLIENT_VERSION, null);
		
		InstalledClient installedClient = mDistribManager.getInstalledClient();
		if (clientVersion == null) {
			if (installedClient.version.length() == 0)	// if empty
				return new InstalledBinary[0];
		}
		if (installedClient.version.length() == 0) { // if not initialized
			// update client version from prefs
			installedClient.version = clientVersion;
			// update distribs
			mDistribManager.save();
		}
		
		ArrayList<InstalledDistrib> installedDistribs = mDistribManager.getInstalledDistribs();
		int installedCount = installedDistribs.size();
		
		InstalledBinary[] result = new InstalledBinary[installedDistribs.size()+1];
		
		result[0] = new InstalledBinary(InstallerService.BOINC_CLIENT_ITEM_NAME, installedClient.version,
				installedClient.description, installedClient.changes);
		
		for (int i = 0; i <installedCount; i++) {
			InstalledDistrib distrib = installedDistribs.get(i);
			result[i+1] = new InstalledBinary(distrib.projectName, distrib.version, distrib.description,
					distrib.changes); 
		}
		
		/* sort result list */
		Arrays.sort(result, 1, result.length, new Comparator<InstalledBinary>() {
			@Override
			public int compare(InstalledBinary bin1, InstalledBinary bin2) {
				return bin1.name.compareTo(bin2.name);
			}
		});
		
		return result;
	}
	
	/**
	 * get list of binaries to update or install
	 * @return update list (sorted)
	 */
	public UpdateItem[] getBinariesToUpdateOrInstall(String[] attachedProjectUrls) {
		ArrayList<UpdateItem> updateItems = new ArrayList<UpdateItem>();
		
		ArrayList<InstalledDistrib> installedDistribs = mDistribManager.getInstalledDistribs();
		
		/* client binary */
		int firstUpdateBoincAppsIndex = 0;
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(mContext);
		String clientVersion = prefs.getString(PreferenceName.CLIENT_VERSION, null);
		
		InstalledClient installedClient = mDistribManager.getInstalledClient();
		
		if (installedClient.version.length()!=0) { // from mDistribManager
			clientVersion = installedClient.version;
		} else { // if not initialized
			installedClient.version = clientVersion;
			// update
			mDistribManager.save();
		}
		
		if (VersionUtil.compareVersion(mClientDistrib.version, clientVersion) > 0) {
			updateItems.add(new UpdateItem(InstallerService.BOINC_CLIENT_ITEM_NAME,
					mClientDistrib.version, null, mClientDistrib.description,
					mClientDistrib.changes, false));
			firstUpdateBoincAppsIndex = 1;
		}
		
		/* project binaries */
		for (InstalledDistrib installedDistrib: installedDistribs) {
			ProjectDistrib selected = null;
			for (ProjectDistrib projectDistrib: mProjectDistribs)
				if (projectDistrib.projectUrl.equals(installedDistrib.projectUrl) &&
						/* if version newest than installed */
						VersionUtil.compareVersion(projectDistrib.version,
						installedDistrib.version) > 0) {
					selected = projectDistrib;
					break;
				}
			
			if (selected != null) {	// if update found
				String filename = null;
				filename = selected.filename;
				
				updateItems.add(new UpdateItem(selected.projectName, selected.version, filename,
						selected.description, selected.changes, false));
			}
		}
		/* new project binaries */
		for (String attachedUrl: attachedProjectUrls) {
			boolean alreadyExists = true;
			for (InstalledDistrib projectDistrib: installedDistribs) {
				// if already exists
				if (projectDistrib.projectUrl.equals(attachedUrl)) {
					alreadyExists = true;
					break;
				}
			}
			if (alreadyExists)
				continue;
			
			ProjectDistrib selected = null;
			for (ProjectDistrib projectDistrib: mProjectDistribs)
				if (projectDistrib.projectUrl.equals(attachedUrl)) {
					selected = projectDistrib;
					break;
				}
			
			updateItems.add(new UpdateItem(selected.projectName, selected.version, selected.filename,
					selected.description, selected.changes, true));
		}
		
		UpdateItem[] array = updateItems.toArray(new UpdateItem[0]);
		Arrays.sort(array, firstUpdateBoincAppsIndex, array.length, new Comparator<UpdateItem>() {
			@Override
			public int compare(UpdateItem updateItem1, UpdateItem updateItem2) {
				return updateItem1.name.compareTo(updateItem2.name);
			}
		});
		return array;
	}
	
	/*
	 *
	 */
	private synchronized void notifyOperation(final String distribName, final String projectUrl,
			final String opDesc) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onOperation(distribName, projectUrl, opDesc);
			}
		});
	}
	
	private synchronized void notifyProgress(final String distribName, final String projectUrl,
			final String opDesc, final int progress) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onOperationProgress(distribName, projectUrl, opDesc, progress);
			}
		});
	}
	
	private synchronized void notifyError(final String distribName, final String projectUrl,
			final String errorMessage) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onOperationError(distribName, projectUrl, errorMessage);
			}
		});
	}
	
	private synchronized void notifyCancel(final String distribName, final String projectUrl) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onOperationCancel(distribName, projectUrl);
			}
		});
	}
	
	private synchronized void notifyFinish(final String distribName, final String projectUrl) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onOperationFinish(distribName, projectUrl);
			}
		});
	}
	
	private synchronized void notifyProjectDistribs(final ArrayList<ProjectDistrib> projectDistribs) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.currentProjectDistribList(projectDistribs);
			}
		});
	}
	
	private synchronized void notifyClientDistrib(final ClientDistrib clientDistrib) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.currentClientDistrib(clientDistrib);
			}
		});
	}

	@Override
	public void onNativeBoincError(String message) {
	}

	@Override
	public synchronized void updatedProjectApps(String projectUrl) {
		ProjectAppsInstaller appsInstaller = mProjectAppsInstallers.remove(projectUrl);
		if (Logging.DEBUG) Log.d(TAG, "After update on client side: "+
				appsInstaller.mProjectDistrib.projectName);
		
		mDistribManager.addOrUpdateDistrib(appsInstaller.mProjectDistrib, appsInstaller.mFileList);
		
		notifyFinish(appsInstaller.mProjectDistrib.projectName, projectUrl);
	}
	
	@Override
	public void updateProjectAppsError(String projectUrl) {
		ProjectAppsInstaller appsInstaller = mProjectAppsInstallers.remove(projectUrl);
		if (Logging.DEBUG) Log.d(TAG, "After update on client side error: "+
				appsInstaller.mProjectDistrib.projectName);
		
		notifyError(appsInstaller.mProjectDistrib.projectName, projectUrl,
				mContext.getString(R.string.updateProjectAppError));
	}

	private static final int INSTALL_PROJECT_DELAY = 200;
	
	@Override
	public void onMonitorEvent(final ClientEvent event) {
		switch(event.type) {
		case ClientEvent.EVENT_ATTACHED_PROJECT:
			postDelayed(new Runnable() {
				/* do install project binaries */
				@Override
				public void run() {
					if (Logging.DEBUG) Log.d(TAG, "attached project "+event.projectUrl);
					
					installProjectApplicationsAutomatically(event.projectUrl);
				}
			}, INSTALL_PROJECT_DELAY);
		case ClientEvent.EVENT_DETACHED_PROJECT:
			// synchronize with installed projects
			post(new Runnable() {
				@Override
				public void run() {
					synchronizeInstalledProjects();
				}
			});
		case ClientEvent.EVENT_RUN_BENCHMARK:
			mBenchmarkIsRun = true;
			break;
		case ClientEvent.EVENT_FINISH_BENCHMARK:
			mBenchmarkIsRun = false;
			// unlock
			notifyIfBenchmarkFinished();
			break;
		}
	}

	@Override
	public void onClientStateChanged(boolean isRun) {
		if (isRun) {
			if (mIsClientBeingInstalled) {
				mDelayedClientShutdownSem.tryAcquire();
			} else
				notifyIfClientUpdatedAndRan();
		} else {
			if (mIsClientBeingInstalled)
				mDelayedClientShutdownSem.release();
		}
	}
}
