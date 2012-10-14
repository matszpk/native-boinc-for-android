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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.HashSet;
import java.util.concurrent.Semaphore;

import sk.boinc.nativeboinc.BoincManagerApplication;
import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
import sk.boinc.nativeboinc.nativeclient.NativeBoincUtils;
import sk.boinc.nativeboinc.util.Chmod;
import sk.boinc.nativeboinc.util.FileUtils;
import sk.boinc.nativeboinc.util.PreferenceName;
import sk.boinc.nativeboinc.util.SDCardExecs;
import sk.boinc.nativeboinc.util.UpdateItem;
import sk.boinc.nativeboinc.util.VersionUtil;

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
	/**
	 * INFO: all operations, can be cancelled with using Thread.interrupt()
	 */
	
	private static final String TAG = "InstallationOps";

	private Context mContext = null;
	private BoincManagerApplication mApp = null;
		
	private static final int NOTIFY_PERIOD = 400;
	private static final int BUFFER_SIZE = 4096;
	
	private String mExternalPath = null; 
	
	private NativeBoincService mRunner = null;
	private Downloader mDownloader = null;
	private InstalledDistribManager mDistribManager = null;
	
	private InstallerHandler mInstallerHandler = null;
	
	private static final String EXECS_NAME = ".__execs__";
	
	public InstallationOps(InstallerHandler installerHandler, Context context, Downloader downloader,
			InstalledDistribManager distribManager) {
		mContext = context;
		mApp = (BoincManagerApplication)mContext.getApplicationContext();
		mDownloader = downloader;
		mInstallerHandler = installerHandler;
		mDistribManager = distribManager;
		mExternalPath = Environment.getExternalStorageDirectory().toString();
	}
	
	public void destroy() {
		mExternalPath = null;
		mRunner = null;
		mDownloader = null;
		mDistribManager = null;
		mInstallerHandler = null;
		mApp = null;
		mContext = null;
	}
	
	public void setRunnerService(NativeBoincService service) {
		mRunner = service;
	}
	
	/*
	 * retrieve client distrib
	 */
	public ClientDistrib updateClientDistrib(int channelId, InstallOp installOp) {
		String clientListUrl = mContext.getString(R.string.installClientSourceUrl)+"client.xml";
		
		ClientDistrib clientDistrib = null;
			
		if (Logging.DEBUG) Log.d(TAG, "updateClientDistrib");
		try {
			/* download boinc client's list */
			try {	/* download with ignoring */
				mDownloader.downloadFile(clientListUrl, "client.xml",
						mContext.getString(R.string.clientListDownload), 
						mContext.getString(R.string.clientListDownloadError), false,
						channelId, installOp, "", "");
				
				if (Thread.interrupted()) { // if cancelled
					if (Logging.DEBUG) Log.d(TAG, "updateClientDistrib interrupted");
					mInstallerHandler.notifyCancel(channelId, installOp, "", "");
					return null;
				}
				
				/* if doesnt downloaded */
				if (!mContext.getFileStreamPath("client.xml").exists())
					return null;
				
				int status = mDownloader.verifyFile(mContext.getFileStreamPath("client.xml"),
						clientListUrl, false, channelId, installOp, "", "");
				
				if (status == Downloader.VERIFICATION_CANCELLED) {
					Thread.interrupted(); // clear interrupted flag
					if (Logging.DEBUG) Log.d(TAG, "updateClientDistrib verif canceled");
					mInstallerHandler.notifyCancel(channelId, installOp, "", "");
					return null;	// cancelled
				}
				if (status == Downloader.VERIFICATION_FAILED) {
					mInstallerHandler.notifyError(channelId, installOp,
							"", "", mContext.getString(R.string.verifySignatureFailed));
					return null;	// cancelled
				}
			} catch(InstallationException ex) {
				return null;
			}

			if (Thread.interrupted()) { // if cancelled
				if (Logging.DEBUG) Log.d(TAG, "updateClientDistrib interrupted");
				mInstallerHandler.notifyCancel(channelId, installOp, "", "");
				return null;
			}
			
			/* parse it */
			InputStream inStream = null;
			
			try {
				inStream = mContext.openFileInput("client.xml");
				/* parse and notify */
				clientDistrib = ClientDistribListParser.parse(inStream);
				if (clientDistrib == null)
					mInstallerHandler.notifyError(channelId, installOp,
							"", "", mContext.getString(R.string.clientListParseError));
				else { // verify data
					if (clientDistrib.filename.length() == 0 || clientDistrib.version.length() == 0)
						mInstallerHandler.notifyError(channelId, installOp, "", "",
								mContext.getString(R.string.badDataInClientDistrib));
				}
			}  catch(IOException ex) {
				mInstallerHandler.notifyError(channelId, installOp, "", "",
						mContext.getString(R.string.clientListParseError));
				return null;
			} finally {
				try {
					inStream.close();
				} catch(IOException ex) { }
			}
			
			return clientDistrib;
		} finally {
			// delete obsolete file
			mContext.deleteFile("client.xml");
		}
	}
	
	/*
	 * retrieve project distrib list
	 */
	
	public ArrayList<ProjectDistrib> parseProjectDistribs(String filename) {
		return parseProjectDistribs(InstallerService.DEFAULT_CHANNEL_ID, filename, false, null);
	}
	
	public ArrayList<ProjectDistrib> parseProjectDistribs(int channelId,
			String filename, boolean notify, InstallOp installOp) {
		InputStream inStream = null;
		ArrayList<ProjectDistrib> projectDistribs = null;
		try {
			inStream = mContext.openFileInput(filename);;
			
			/* parse and notify */
			projectDistribs = ProjectDistribListParser.parse(inStream);
			if (projectDistribs != null) {
				for (ProjectDistrib distrib :projectDistribs) {
					if (distrib.projectName.length() == 0 ||
							// we dont check filename (handling project with builtin Android apps
							distrib.projectUrl.length() == 0 ||
							// check version when filename is specified
							(distrib.version.length() == 0 && distrib.filename.length() != 0)) {
						if (notify) // notify error (corrupted list)
							mInstallerHandler.notifyError(channelId, installOp, "", "",
									mContext.getString(R.string.badDataInProjectDistribs));
						return null;
					}
				}
				
				// if success
				return projectDistribs;
			} else if (notify) // notify error
				mInstallerHandler.notifyError(channelId, installOp, "", "",
						mContext.getString(R.string.appListParseError));
		} catch(IOException ex) {
			if (notify)
				mInstallerHandler.notifyError(channelId, installOp, "", "",
						mContext.getString(R.string.appListParseError));
		} finally {
			try {
				if (inStream != null) inStream.close();
			} catch(IOException ex) { }
		}
		// failed
		return null;
	}
	
	/**
	 * @return null if cancelled
	 */
	public ArrayList<ProjectDistrib> updateProjectDistribList(int channelId, InstallOp installOp,
			ArrayList<ProjectDistrib> previousDistribs) {
		String appListUrl = mContext.getString(R.string.installAppsSourceUrl)+"apps2.xml";
		
		try {
			mDownloader.downloadFile(appListUrl, "apps.xml",  // new file, we break compatibility in this version
					mContext.getString(R.string.appListDownload),
					mContext.getString(R.string.appListDownloadError), false, channelId, 
					installOp, "", "");
			
			if (Thread.interrupted()) { // if cancelled
				if (Logging.DEBUG) Log.d(TAG, "updateClientDistrib interrupted");
				mInstallerHandler.notifyCancel(channelId, installOp, "", "");
				return null;
			}
			
			int status = mDownloader.verifyFile(mContext.getFileStreamPath("apps.xml"),
					appListUrl, false, channelId, installOp, "", "");
			
			if (status == Downloader.VERIFICATION_CANCELLED) {
				Thread.interrupted(); // clear interrupted flag
				mInstallerHandler.notifyCancel(channelId, installOp, "", "");
				return null;	// cancelled
			}
			if (status == Downloader.VERIFICATION_FAILED) {
				mInstallerHandler.notifyError(channelId, installOp, "", "",
						mContext.getString(R.string.verifySignatureFailed));
				/* errors occurred, but we returns previous value */
				return previousDistribs;
			}
			
			if (Thread.interrupted()) { // if cancelled
				mInstallerHandler.notifyCancel(channelId, installOp, "", "");
				return null;
			}
			
			/* parse it */
			ArrayList<ProjectDistrib> projectDistribs = parseProjectDistribs(channelId, "apps.xml", true,
					installOp);
			if (projectDistribs != null) {
				// make backup
				mContext.getFileStreamPath("apps.xml").renameTo(
						mContext.getFileStreamPath("apps-old.xml"));			
			}
			return projectDistribs;
		} catch(InstallationException ex) {
			/* errors occurred, but we returns previous value */
			return previousDistribs;
		} finally {
			// delete obsolete file
			mContext.deleteFile("apps.xml");
		}
	}
	
	/*
	 * get installed binaries
	 */
	public InstalledBinary[] getInstalledBinaries() {
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(mContext);
		String clientVersion = prefs.getString(PreferenceName.CLIENT_VERSION, null);
		
		InstalledClient installedClient = mDistribManager.getInstalledClient();
		if (clientVersion == null) {
			if (installedClient.version.length() == 0 && !installedClient.fromSDCard)	// if empty
				return new InstalledBinary[0];
		}
		if (installedClient.version.length() == 0 && !installedClient.fromSDCard) { // if not initialized
			// update client version from prefs
			installedClient.version = clientVersion;
			// update distribs
			mDistribManager.save();
		}
		
		ArrayList<InstalledDistrib> installedDistribs = mDistribManager.getInstalledDistribs();
		int installedCount = installedDistribs.size();
		
		InstalledBinary[] result = new InstalledBinary[installedDistribs.size()+1];
		
		result[0] = new InstalledBinary(InstallerService.BOINC_CLIENT_ITEM_NAME, installedClient.version,
				installedClient.description, installedClient.changes, installedClient.fromSDCard);
		
		for (int i = 0; i <installedCount; i++) {
			InstalledDistrib distrib = installedDistribs.get(i);
			result[i+1] = new InstalledBinary(distrib.projectName, distrib.version, distrib.description,
					distrib.changes, distrib.fromSDCard); 
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
	 * get list of binaries to update or install (from project url)
	 * @return update list (sorted)
	 */
	public UpdateItem[] getBinariesToUpdateOrInstall(ClientDistrib clientDistrib,
			ArrayList<ProjectDistrib> projectDistribs, String[] attachedProjectUrls) {
		
		ArrayList<InstalledDistrib> installedDistribs = mDistribManager.getInstalledDistribs();
		ArrayList<UpdateItem> updateItems = new ArrayList<UpdateItem>();
		
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
		
		/* always update when client from sdcard */
		if ((installedClient != null && installedClient.fromSDCard) ||
				VersionUtil.compareVersion(clientDistrib.version, clientVersion) > 0) {
			updateItems.add(new UpdateItem(InstallerService.BOINC_CLIENT_ITEM_NAME,
					clientDistrib.version, null, clientDistrib.description,
					clientDistrib.changes, false));
			firstUpdateBoincAppsIndex = 1;
		}
		
		/* project binaries */
		for (InstalledDistrib installedDistrib: installedDistribs) {
			ProjectDistrib selected = null;
			for (ProjectDistrib projectDistrib: projectDistribs)
				if (projectDistrib.projectUrl.equals(installedDistrib.projectUrl) &&
						/* if version newest than installed or distrib from sdcard */
						(installedDistrib.fromSDCard ||
						 VersionUtil.compareVersion(projectDistrib.version, installedDistrib.version) > 0)) {
					selected = projectDistrib;
					break;
				}
			
			if (selected != null && selected.filename != null && selected.filename.length()!=0) // if update found
				updateItems.add(new UpdateItem(selected.projectName, selected.version, selected.filename,
						selected.description, selected.changes, false));
		}
		/* new project binaries */
		for (String attachedUrl: attachedProjectUrls) {
			boolean alreadyExists = false;
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
			for (ProjectDistrib projectDistrib: projectDistribs)
				if (projectDistrib.projectUrl.equals(attachedUrl)) {
					selected = projectDistrib;
					break;
				}
			
			if (selected != null && selected.filename != null && selected.filename.length()!=0)
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
	
	public String[] getBinariesToUpdateFromSDCard(int channelId, InstallOp installOp, String path,
			ProjectDescriptor[] projectDescs) {
		File dirFile = new File(path);
		if (!dirFile.isDirectory()) {
			mInstallerHandler.notifyError(channelId, installOp, "", "",
					mContext.getString(R.string.binSDCardDirReadError));
			return null;
		}
		
		File[] fileList = dirFile.listFiles();
		if (fileList == null) {
			mInstallerHandler.notifyError(channelId, installOp, "", "",
					mContext.getString(R.string.binSDCardDirReadError));
			return null;
		}
		
		ArrayList<InstalledDistrib> installedDistribs = mDistribManager.getInstalledDistribs();
		ArrayList<String> distribsToUpdate = new ArrayList<String>(1);
		
		/* for projects */
		for (File file: fileList) {
			String distribName;
			
			String filename = file.getName();
			
			if (filename.equals("boinc_client") || filename.equals("boinc_client.zip")) {
				// if boinc client
				distribsToUpdate.add(InstallerService.BOINC_CLIENT_ITEM_NAME);
				continue;
			}
			
			if (file.isDirectory()) // if directory
				distribName = filename;
			else if (filename.endsWith(".zip")) // if zip
				distribName = filename.substring(0, filename.length() - 4);
			else // if not determined
				continue;
			
			boolean found = false;
			for (InstalledDistrib distrib: installedDistribs)
				if (distrib.projectName.equals(distribName)) {
					distribsToUpdate.add(distribName);
					found = true;
					break;
				}
			
			if (!found && projectDescs != null) { // search in project boinc list
				// handling project desc.projectName is null
				for (ProjectDescriptor desc: projectDescs)
					if (distribName.equals(desc.projectName)) {
						distribsToUpdate.add(distribName);
						break;
					}
			}
		}
		String[] output = distribsToUpdate.toArray(new String[0]);
		Arrays.sort(output);
		return output;
	}
	
	/*
	 * Dump boinc files routines
	 */
	private long calculateDirectorySize(File dir) {
		int length = 0;
		File[] dirFiles = dir.listFiles();
		if (dirFiles == null)
			return -1;
		
		for (File file: dirFiles) {
			if (file.getName().equals(EXECS_NAME))
				continue; // skip '.__execs__' file
			
			if (file.isDirectory()) {
				long childLength = calculateDirectorySize(file);
				if (childLength == -1)
					return -1;
				length += childLength;
			}
			else
				length += file.length();
			
			// keep interrupt state for parent method
			if (Thread.currentThread().isInterrupted()) // if cancelled
				return length;
		}
		return length;
	}
	
	private byte[] mBuffer = new byte[BUFFER_SIZE];
	
	private long mPreviousNotifyTime;
	
	private StringBuilder mStringBuilder = new StringBuilder();
	
	/* copyDir modes */
	private static final int COPY_NORMALLY = 0;
	private static final int COPY_TO_SDCARD = 1;
	private static final int COPY_FROM_SDCARD = 2;
	
	private static interface ProgressNotifier {
		public void notifyProgress(int progress);
	}
	
	public class CopyDirectoryHelper {
		
		private final long mInputSize;
		private final int mMode;
		private final ProgressNotifier mProgressNotifier;
		
		public CopyDirectoryHelper(long inputSize, int mode, ProgressNotifier progressNotifier) {
			this.mInputSize = inputSize;
			this.mMode = mode;
			this.mProgressNotifier = progressNotifier;
		}
		
		public long copyDirectory(File inDir, File outDir, long copied) throws IOException {
			File[] inDirFiles = inDir.listFiles();
			
			if (inDirFiles == null)
				throw new IOException("Cant access to directory!");
			
			for (File file: inDirFiles) {
				if (file.getName().equals(EXECS_NAME))
					continue; // skip '.__execs__' file
				
				mStringBuilder.setLength(0);
				mStringBuilder.append(outDir.getPath());
				mStringBuilder.append("/");
				mStringBuilder.append(file.getName());
				File outFile = new File(mStringBuilder.toString());
				
				if (file.isDirectory()) {
					if (!outFile.exists())
						outFile.mkdir();
					
					copied = copyDirectory(file, outFile, copied);
					
					// keep interrupt state for parent method
					if (Thread.currentThread().isInterrupted()) // cancel
						return copied;
				} else {
					FileInputStream inStream = null;
					FileOutputStream outStream = null;
					
					try {
						inStream = new FileInputStream(file);
						outStream = new FileOutputStream(outFile);
						
						// keep interrupt state for parent method
						if (Thread.currentThread().isInterrupted()) // cancel
							return copied;
						
						while(true) {
							/* main copy routine */
							int readed = inStream.read(mBuffer);
							
							copied += readed;
							
							long currentNotifyTime = System.currentTimeMillis();
							if (currentNotifyTime-mPreviousNotifyTime>NOTIFY_PERIOD) {
								// notify about progress
								mProgressNotifier.notifyProgress((int)(10000.0*(double)copied/(double)mInputSize));
								mPreviousNotifyTime = currentNotifyTime;
								
								if (Thread.currentThread().isInterrupted()) // cancel
									return copied;
							}
							
							if (readed == -1)
								break;
							outStream.write(mBuffer, 0, readed);
						}
						
						outStream.flush();
					} finally {
						if (inStream != null)
							inStream.close();
						if (outStream != null)
							outStream.close();
					}
				}
			}
			
			int execsFd = -1;
			try {
				if (mMode == COPY_FROM_SDCARD) {
					execsFd = SDCardExecs.openExecsLock(inDir.getPath(), false);
					HashSet<String> execsNames = new HashSet<String>();
					SDCardExecs.readExecs(execsFd, execsNames);
					
					mStringBuilder.setLength(0);
					mStringBuilder.append(outDir.getPath());
					mStringBuilder.append("/");
					int lastSlashIndex = mStringBuilder.length();
					
					/* grant permissions for destination files (in __execs__ file) */
					for (String execName: execsNames) {
						mStringBuilder.append(execName);
						Chmod.chmod(mStringBuilder.toString(), 0700);
						mStringBuilder.delete(lastSlashIndex, mStringBuilder.length());
					}
				} else if (mMode == COPY_TO_SDCARD) {
					execsFd = SDCardExecs.openExecsLock(outDir.getPath(), true);
					/* grant permissions for destination files (from __execs__ file) */
					for (File inFile: inDirFiles) {
						if (inFile.isDirectory())
							continue;
						
						int filemode = Chmod.getmod(inFile.getPath());
						if (filemode != -1)
							SDCardExecs.setExecMode(execsFd, inFile.getName(), (filemode & 0111) != 0);
					}
				}
			} finally {
				SDCardExecs.closeExecsLock(execsFd);
			}
			return copied;
		}
	}
	
	public static boolean isDestinationExists(String directory) {
		String externalPath = Environment.getExternalStorageDirectory().toString();
		String outPath = FileUtils.joinBaseAndPath(externalPath, directory);
		
		return new File(outPath).exists();
	}
	
	/**
	 * main routine of dumping boinc files into sdcard
	 * @param directory
	 */
	public void dumpBoincFiles(String directory) {
		String outPath = FileUtils.joinBaseAndPath(mExternalPath, directory);
		
		if (Logging.DEBUG) Log.d(TAG, "DumpBoinc: OutPath:"+outPath);
		
		File fileDir = new File(outPath);
		deleteDirectory(fileDir);
		fileDir.mkdirs();
				
		File boincDir = new File(mInstallerHandler.getInstallPlacePath());
		long boincDirSize = calculateDirectorySize(boincDir);
		if (boincDirSize == -1) {
			mInstallerHandler.notifyError(InstallerService.BOINC_DUMP_ITEM_NAME, "",
							mContext.getString(R.string.unexpectedError));
			return;
		}
		
		if (Thread.interrupted()) {
			mInstallerHandler.notifyCancel(InstallerService.BOINC_DUMP_ITEM_NAME, "");
			return;
		}
		
		mPreviousNotifyTime = System.currentTimeMillis();
		try {
			new CopyDirectoryHelper(boincDirSize, COPY_NORMALLY, new ProgressNotifier() {
						@Override
						public void notifyProgress(int progress) {
							mInstallerHandler.notifyProgress(InstallerService.BOINC_DUMP_ITEM_NAME, "",
									mContext.getString(R.string.dumpBoincProgress), progress);
						}
					}).copyDirectory(boincDir, fileDir, 0);
			
			if (Thread.interrupted()) {
				mInstallerHandler.notifyCancel(InstallerService.BOINC_DUMP_ITEM_NAME, "");
				return;
			} else
				mInstallerHandler.notifyFinish(InstallerService.BOINC_DUMP_ITEM_NAME, "");
		} catch(IOException ex) {
			mInstallerHandler.notifyError(InstallerService.BOINC_DUMP_ITEM_NAME, "", ex.getMessage());
		}
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
			
			// keep interrupt state for parent method
			if (Thread.currentThread().isInterrupted())
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
		if (mRunner.isRun()) {
			mInstallerHandler.notifyOperation(InstallerService.BOINC_REINSTALL_ITEM_NAME, "",
					mContext.getString(R.string.waitingForShutdown));
			
			mShutdownClientSem.tryAcquire();
			
			mRunner.shutdownClient();
			// awaiting for shutdown
			try {
				mShutdownClientSem.acquire();
			} catch(InterruptedException ex) {
				mInstallerHandler.notifyCancel(InstallerService.BOINC_REINSTALL_ITEM_NAME, "");
				return;
			} finally {
				mShutdownClientSem.release();
			}
		}
		
		deleteDirectory(new File(mInstallerHandler.getInstallPlacePath()));
		
		if (Thread.interrupted()) {
			mInstallerHandler.notifyCancel(InstallerService.BOINC_REINSTALL_ITEM_NAME, "");
			return;
		}
		
		new File(mInstallerHandler.getInstallPlacePath()).mkdirs();
		
		// now reinitialize boinc directory (first start and other operation) */
		try {
			mInstallerHandler.notifyOperation(InstallerService.BOINC_REINSTALL_ITEM_NAME, "",
					mContext.getString(R.string.nativeClientFirstStart));
			
			if (!NativeBoincService.firstStartClient(mContext)) {
				mInstallerHandler.notifyError(InstallerService.BOINC_REINSTALL_ITEM_NAME, "",
								mContext.getString(R.string.nativeClientFirstStartError));
		    	return;
		    }
			mInstallerHandler.notifyOperation(InstallerService.BOINC_REINSTALL_ITEM_NAME, "",
					mContext.getString(R.string.nativeClientFirstKill));
			
			if (Thread.interrupted()) {
				mInstallerHandler.notifyCancel(InstallerService.BOINC_REINSTALL_ITEM_NAME, "");
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
	    	
	    	if (Thread.interrupted()) {
				mInstallerHandler.notifyCancel(InstallerService.BOINC_REINSTALL_ITEM_NAME, "");
				return;
			}
			
	    	// 
	    	mInstallerHandler.synchronizeInstalledProjects();
	    	
	    	mApp.setRunRestartAfterReinstall(); // inform runner
			mRunner.startClient(true);
	    	mInstallerHandler.notifyFinish(InstallerService.BOINC_REINSTALL_ITEM_NAME, "");
		} catch(Exception ex) {
			if (Logging.ERROR) Log.e(TAG, "on client install finishing:"+
					ex.getClass().getCanonicalName()+":"+ex.getMessage());
			mInstallerHandler.notifyError(InstallerService.BOINC_REINSTALL_ITEM_NAME, "",
							mContext.getString(R.string.unexpectedError));
		}
	}
	
	/**
	 * deleting project binaries (from app_info.xml)
	 */
	
	public void deleteProjectBinaries(int channelId, ArrayList<String> projectNames) {
		String projectsPath = mInstallerHandler.getInstallPlacePath()+"/projects/";
		
		for (String projectName: projectNames) {
			InstalledDistrib found = null;
			for (InstalledDistrib distrib: mDistribManager.getInstalledDistribs())
				if (distrib.projectName.equals(projectName)) {
					found = distrib;
					break;
				}
			
			if (found != null) { // if found
				String projPath = projectsPath+InstallerHandler.escapeProjectUrl(found.projectUrl);
				FileInputStream inStream = null;
				ArrayList<String> filenames = null;
				File appInfoFile = new File(projPath, "app_info.xml");
				try {
					inStream = new FileInputStream(appInfoFile);
					filenames = ExecFilesAppInfoParser.parse(inStream);
				} catch(IOException ex) {
					mInstallerHandler.notifyError(channelId, InstallOp.DeleteProjectBinaries, "", "",
							mContext.getString(R.string.cantDeleteProjectBins));
					return;
				} finally {
					try {
						if (inStream!=null)
							inStream.close();
					} catch(IOException ex2) { }
				}
				
				// deleting files
				int execsFd = -1;
				
				if (!mInstallerHandler.isInstallOnSDCard()) {
					for (String filename: filenames)
						new File(projPath,filename).delete();
				} else {
					/* removing files and granted permissions */
					try {
						execsFd = SDCardExecs.openExecsLock(projPath, true);
						
						for (String filename: filenames) {
							SDCardExecs.setExecMode(execsFd, filename, false);
							new File(projPath,filename).delete();
						}
						SDCardExecs.setExecMode(execsFd, "app_info.xml", false);
					} catch(IOException ex) {
						mInstallerHandler.notifyError(channelId, InstallOp.DeleteProjectBinaries, "", "",
								mContext.getString(R.string.cantDeleteProjectBins));
					} finally {
						SDCardExecs.closeExecsLock(execsFd);
					}
				}
				appInfoFile.delete();
			}
		}
		// update distribs list
		mDistribManager.removeDistribsByProjectName(projectNames);
	}
	
	/*
	 * move installation from/to sdcard
	 */
	public void moveInstallationTo() {
		boolean runBoinc = false;
		
		if (mRunner.isRun()) {
			runBoinc = true;
			mInstallerHandler.notifyOperation(InstallerService.BOINC_MOVETO_ITEM_NAME, "",
					mContext.getString(R.string.waitingForShutdown));
			
			mShutdownClientSem.tryAcquire();
			
			mRunner.shutdownClient();
			// awaiting for shutdown
			try {
				mShutdownClientSem.acquire();
			} catch(InterruptedException ex) {
				mInstallerHandler.notifyCancel(InstallerService.BOINC_MOVETO_ITEM_NAME, "");
				return;
			} finally {
				mShutdownClientSem.release();
			}
		}
		
		if (Thread.interrupted()) {
			mInstallerHandler.notifyCancel(InstallerService.BOINC_MOVETO_ITEM_NAME, "");
			return;
		}
		
		String intMemPath = BoincManagerApplication.getBoincDirectory(mContext, false);
		String sdCardPath = BoincManagerApplication.getBoincDirectory(mContext, true);
		File intMemDir = new File(intMemPath);
		File sdCardDir = new File(sdCardPath);
		
		if (mInstallerHandler.isInstallOnSDCard()) { // move to internal memory
			long boincDirSize = calculateDirectorySize(sdCardDir);
			
			if (!intMemDir.isDirectory())
				intMemDir.delete();
			else // delete existing directory
				deleteDirectory(intMemDir);
			intMemDir.mkdirs();
			
			try {
				new CopyDirectoryHelper(boincDirSize, COPY_FROM_SDCARD, new ProgressNotifier() {
					@Override
					public void notifyProgress(int progress) {
						mInstallerHandler.notifyProgress(InstallerService.BOINC_MOVETO_ITEM_NAME, "",
								mContext.getString(R.string.moveToProgress), progress);
					}
				}).copyDirectory(sdCardDir, intMemDir, 0);
				
				if (Thread.interrupted()) {
					deleteDirectory(intMemDir); // delete if cancelled
					mInstallerHandler.notifyCancel(InstallerService.BOINC_MOVETO_ITEM_NAME, "");
					return;
				}
			} catch(IOException ex) {
				deleteDirectory(intMemDir);
				mInstallerHandler.notifyError(InstallerService.BOINC_MOVETO_ITEM_NAME, "", ex.getMessage());
				return;
			}
			// delete previous place
			deleteDirectory(sdCardDir);
		} else { // move to sdcard memory
			long boincDirSize = calculateDirectorySize(intMemDir);
			
			if (!sdCardDir.isDirectory())
				sdCardDir.delete();
			else // delete previous version
				deleteDirectory(sdCardDir);
			sdCardDir.mkdirs();
			
			try {
				new CopyDirectoryHelper(boincDirSize, COPY_TO_SDCARD, new ProgressNotifier() {
					@Override
					public void notifyProgress(int progress) {
						mInstallerHandler.notifyProgress(InstallerService.BOINC_MOVETO_ITEM_NAME, "",
								mContext.getString(R.string.moveToProgress), progress);
					}
				}).copyDirectory(intMemDir, sdCardDir, 0);
				
				if (Thread.interrupted()) {
					deleteDirectory(sdCardDir); // delete if cancelled
					mInstallerHandler.notifyCancel(InstallerService.BOINC_MOVETO_ITEM_NAME, "");
					return;
				}
			} catch(IOException ex) {
				deleteDirectory(sdCardDir);
				mInstallerHandler.notifyError(InstallerService.BOINC_MOVETO_ITEM_NAME, "", ex.getMessage());
				return;
			}
			// delete previous place
			deleteDirectory(intMemDir);
		}
		
		if (Thread.interrupted()) {
			mInstallerHandler.notifyCancel(InstallerService.BOINC_MOVETO_ITEM_NAME, "");
			return;
		}
		
		/* change preferences (install place) */
		BoincManagerApplication.setBoincPlace(mContext, !mInstallerHandler.isInstallOnSDCard());
		
		if (runBoinc) { // if previusly run, we run now
			mApp.setRunRestartAfterReinstall(); // inform runner
			mRunner.startClient(false);
		}
		
		mInstallerHandler.notifyFinish(InstallerService.BOINC_MOVETO_ITEM_NAME, "");
	}
}
