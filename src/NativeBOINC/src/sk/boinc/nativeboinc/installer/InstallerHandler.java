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
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import edu.berkeley.boinc.lite.Project;
import edu.berkeley.boinc.nativeboinc.ClientEvent;

import sk.boinc.nativeboinc.BoincManagerApplication;
import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.MonitorListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincProjectsListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
import sk.boinc.nativeboinc.nativeclient.NativeBoincStateListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincUpdateListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincUtils;
import sk.boinc.nativeboinc.util.Chmod;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.HostListDbAdapter;
import sk.boinc.nativeboinc.util.PreferenceName;
import sk.boinc.nativeboinc.util.RuntimeUtils;
import sk.boinc.nativeboinc.util.UpdateItem;
import sk.boinc.nativeboinc.util.VersionUtil;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.util.Log;

/**
 * @author mat
 *
 */
public class InstallerHandler extends Handler implements NativeBoincUpdateListener,
	NativeBoincStateListener, NativeBoincProjectsListener, MonitorListener {
	
	private final static String TAG = "InstallerHandler";

	private Context mContext = null;
	private BoincManagerApplication mApp = null;
	
	private InstallerService.ListenerHandler mListenerHandler;
	
	private InstalledDistribManager mDistribManager = null;
	private ArrayList<ProjectDistrib> mProjectDistribs = null;
	private ClientDistrib mClientDistrib = null;
	
	private InstallationOps mInstallOps = null; 
	
	private static final int NOTIFY_PERIOD = 200; /* milliseconds */
	
	private NativeBoincService mRunner = null;
	
	private boolean mIsClientBeingInstalled = false;
	private Semaphore mDelayedClientShutdownSem = new Semaphore(1);
	
	private boolean mDoDumpBoincFiles = false;
	private boolean mDoBoincReinstall = false;
	
	private Downloader mDownloader = null;
	
	private ExecutorService mExecutorService = null;
	
	// if handler is working
	private boolean mHandlerIsWorking = false;
	
	// if installer service is working (previous state)
	private boolean mPreviousStateOfIsWorking = false;
	
	public InstallerHandler(final Context context, InstallerService.ListenerHandler listenerHandler) {
		mContext = context;
		
		mApp = (BoincManagerApplication)context.getApplicationContext();
		
		Log.d(TAG, "Number of processors:"+ RuntimeUtils.getRealCPUCount());
		mExecutorService = Executors.newFixedThreadPool(RuntimeUtils.getRealCPUCount());
		
		mDownloader = new Downloader(context, listenerHandler);
		mInstallOps = new InstallationOps(this, context, listenerHandler);
		mListenerHandler = listenerHandler;
		mDistribManager = new InstalledDistribManager(context);
		mDistribManager.load();
		
		// parse project distribs
		parseProjectDistribs("apps-old.xml", false, false);
	}
	
	public void destroy() {
		mExecutorService.shutdown();
		try {
			mExecutorService.awaitTermination(5, TimeUnit.SECONDS);
			mExecutorService.shutdownNow();
			mExecutorService.awaitTermination(5, TimeUnit.SECONDS);
		} catch (InterruptedException ex) {
			mExecutorService.shutdownNow();
		}
	}
	
	public void setRunnerService(NativeBoincService service) {
		mRunner = service;
		mInstallOps.setRunnerService(service);
	}
	
	public String resolveProjectName(String projectUrl) {
		if (mProjectDistribs == null)
			return null;
		
		for (ProjectDistrib distrib: mProjectDistribs)
			if (distrib.projectUrl.equals(projectUrl))
				return distrib.projectName;
		return null;
	}
	
	/**
	 * @return true if success
	 **/
	public boolean updateClientDistrib(boolean notifyResult) {
		synchronized (this) {
			mHandlerIsWorking = true;
			notifyChangeOfIsWorking();
		}
		
		String clientListUrl = mContext.getString(R.string.installClientSourceUrl)+"client.xml";
		
		try {
			mClientDistrib = null;
			
			if (Logging.DEBUG) Log.d(TAG, "updateClientDistrib");
			
			/* download boinc client's list */
			try {	/* download with ignoring */
				mDownloader.downloadFile(clientListUrl, "client.xml",
						mContext.getString(R.string.clientListDownload), 
						mContext.getString(R.string.clientListDownloadError), false, "", "");
				
				/* if doesnt downloaded */
				if (!mContext.getFileStreamPath("client.xml").exists())
					return false;
				
				int status = mDownloader.verifyFile(mContext.getFileStreamPath("client.xml"),
						clientListUrl, false, "", "");
				
				if (status == Downloader.VERIFICATION_CANCELLED) {
					Thread.interrupted(); // clear interrupted flag
					if (Logging.DEBUG) Log.d(TAG, "updateClientDistrib verif canceled");
					mContext.deleteFile("client.xml");
					notifyCancel("", "");
					return false;	// cancelled
				}
				if (status == Downloader.VERIFICATION_FAILED) {
					notifyError("", "", mContext.getString(R.string.verifySignatureFailed));
					return false;	// cancelled
				}
			} catch(InstallationException ex) {
				mContext.deleteFile("client.xml");
				return false;
			}

			if (Thread.interrupted()) { // if cancelled
				if (Logging.DEBUG) Log.d(TAG, "updateClientDistrib interrupted");
				mContext.deleteFile("client.xml");
				return false;
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
			}  catch(IOException ex) {
				notifyError("", "", mContext.getString(R.string.clientListParseError));
				return false;
			} finally {
				try {
					inStream.close();
				} catch(IOException ex) { }
				
				/* delete obsolete file */
				mContext.deleteFile("client.xml");
			}
			
			if (notifyResult)
				notifyClientDistrib(mClientDistrib);
		} finally {
			synchronized (this) {
				mHandlerIsWorking = false;
				notifyChangeOfIsWorking();
			}
		}
		return true;
	}
	
	private static final int BUFFER_SIZE = 4096;
	
	private ClientInstaller mClientInstaller = null;
	private Future<?> mClientInstallerFuture = null;
	
	private boolean mClientShouldBeRun = false;
	
	private class ClientInstaller implements Runnable {
		private boolean mStandalone;
		
		private String mSDCardPath = null;
		
		private boolean mIsRan = false;
		
		public ClientInstaller(boolean standalone, String sdCardPath) {
			mStandalone = standalone;
			mSDCardPath = sdCardPath;
		}
		
		public boolean isRan() {
			return mIsRan;
		}
		
		@Override
		public void run() {
			Thread currentThread = Thread.currentThread();
			
			boolean clientToUpdate = InstallerService.isClientInstalled(mContext);
			
			if (mStandalone)	{// if standalone called from InstallerService
				if (Logging.DEBUG) Log.d(TAG, "Install client standalone");
			}
			
			boolean previouslyClientIsRan = mRunner.isRun();
			String boincClientFilename = "boinc_client";
			if (previouslyClientIsRan) {
				if (Logging.DEBUG) Log.d(TAG, "Install client when ran.");
				mDelayedClientShutdownSem.tryAcquire();
				mRunner.shutdownClient();
				boincClientFilename = "boinc_client_new";
			}
			
			// should be run during installation
			mClientShouldBeRun = previouslyClientIsRan || !clientToUpdate;
			
			try {
				String zipFilename = null;
				
				mIsRan = true;
				
				if (currentThread.isInterrupted() ||
						(mClientInstallerFuture != null && mClientInstallerFuture.isCancelled())) {
					notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
					return;
				}
				
				/* download and unpack */
				if (mSDCardPath == null) {
					if (mClientDistrib == null) {
						notifyError(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
								mContext.getString(R.string.clientDistribNotFound));
						return;
					}
					
					zipFilename = mClientDistrib.filename;
					if (Logging.INFO) Log.i(TAG, "Use zip "+zipFilename);
					
					if (currentThread.isInterrupted()) {
						notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
						return;
					}
					
					// download from server
					try {
						String zipUrlString = mContext.getString(R.string.installClientSourceUrl)+zipFilename; 
						mDownloader.downloadFile(zipUrlString, "boinc_client.zip",
								mContext.getString(R.string.downloadNativeClient),
								mContext.getString(R.string.downloadNativeClientError), true,
								InstallerService.BOINC_CLIENT_ITEM_NAME, "");
						
						if (currentThread.isInterrupted()) {
							mContext.deleteFile("boinc_client.zip");
							notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
							return;
						}
						
						int status = mDownloader.verifyFile(mContext.getFileStreamPath("boinc_client.zip"),
								zipUrlString, true, InstallerService.BOINC_CLIENT_ITEM_NAME, "");
						
						if (status == Downloader.VERIFICATION_CANCELLED) {
							mContext.deleteFile("boinc_client.zip");
							notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
							return;	// cancelled
						}
						if (status == Downloader.VERIFICATION_FAILED) {
							notifyError(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
									mContext.getString(R.string.verifySignatureFailed));
							return;	// cancelled
						}
					} catch(InstallationException ex) {
						/* remove zip file */
						mContext.deleteFile("boinc_client.zip");
						return;
					}
				}
				
				if (currentThread.isInterrupted()) {
					mContext.deleteFile("boinc_client.zip");
					notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
					return;
				}
				
				
				if (mSDCardPath == null || mSDCardPath.endsWith(".zip")) {
					// unzip if path to zip or zip from server
					FileOutputStream outStream = null;
					InputStream zipStream = null;
					ZipFile zipFile = null;
					/* unpack zip file */
					try {
						if (mSDCardPath == null)
							zipFile = new ZipFile(mContext.getFileStreamPath("boinc_client.zip"));
						else // if from sdcard (zip)
							zipFile = new ZipFile(mSDCardPath);
						
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
							
							if (currentThread.isInterrupted()) {
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
						if (mSDCardPath == null) // if from server
							mContext.deleteFile("boinc_client.zip");
					}
				} else {
					// copy to destination
					FileInputStream inputStream = null;
					FileOutputStream outputStream = null;
					
					try {
						inputStream = new FileInputStream(mSDCardPath);
						outputStream = mContext.openFileOutput(boincClientFilename, Context.MODE_PRIVATE);
						
						byte[] buffer = new byte[BUFFER_SIZE];
						
						while (true) {
							int readed = inputStream.read(buffer);
							if (readed == -1)
								break;
							outputStream.write(buffer, 0, readed);
						}
						
					} catch(IOException ex) {
						notifyError(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
								mContext.getString(R.string.copyNativeClientError));
						return;
					} finally {
						try {
							if (inputStream != null)
								inputStream.close();
						} catch(IOException ex) { }
						
						try {
							if (outputStream != null)
								outputStream.close();
						} catch(IOException ex) { }
					}
				}
				
				if (currentThread.isInterrupted()) {
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
				
			    if (currentThread.isInterrupted()) {
					notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
					return;
				}
			    
			    /* create boinc directory */
			    File boincDir = new File(mContext.getFilesDir()+"/boinc");
			    if (!boincDir.isDirectory()) {
			    	boincDir.delete();
			    	boincDir.mkdir();
			    }
			    
			    if (mSDCardPath == null) {
				    if (mClientDistrib != null)
			    		mDistribManager.setClient(mClientDistrib);
			    } else // from SDCard
		    		mDistribManager.setClientFromSDCard();
			    
			    /* running and killing boinc client (run in this client) */
			    if (!clientToUpdate) {
			    	if (Logging.DEBUG) Log.d(TAG, "First start client");
			    	
			    	notifyOperation(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
			    			mContext.getString(R.string.nativeClientFirstStart));
			    	
				    if (!NativeBoincService.firstStartClient(mContext)) {
				    	if (Logging.DEBUG) Log.d(TAG, "first start client error");
				    	notifyError(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
				    			mContext.getString(R.string.nativeClientFirstStartError));
				    	return;
				    }
				    notifyOperation(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
			    			mContext.getString(R.string.nativeClientFirstKill));	    	
			    }
			    
			    if (previouslyClientIsRan) {
			    	if (Logging.DEBUG) Log.d(TAG, "Rename to real boinc");
			    	try {
			    		notifyOperation(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
			    				mContext.getString(R.string.nativeClientAwaitForShutdown));
				    	mDelayedClientShutdownSem.acquire();
				    	// rename new boinc client file
				    	mContext.getFileStreamPath("boinc_client_new").renameTo(
				    			mContext.getFileStreamPath("boinc_client"));
				    	/* give control over installation to NativeBoincService (second start) */
				    	mIsClientBeingInstalled = false;
				    	
				    	mApp.setRunRestartAfterReinstall(); // inform runner
			    		mRunner.startClient(false);
				    	
				    	mDelayedClientShutdownSem.release();
			    	} catch(InterruptedException ex) {
			    		notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
						return;
			    	}
			    } else {
			    	if (!clientToUpdate) {	// if should be installed first
			    		// add nativeboinc to client list
				    	HostListDbAdapter dbAdapter = null;
				    	
				    	if (Logging.DEBUG) Log.d(TAG, "Adding nativeboinc to hostlist");
					    
				    	// set up waiting for benchmark
						SharedPreferences sharedPrefs = PreferenceManager
								.getDefaultSharedPreferences(mContext);
						sharedPrefs.edit().putBoolean(PreferenceName.WAITING_FOR_BENCHMARK, true)
								.commit();
						
				    	try {
				    		String accessPassword = NativeBoincUtils.getAccessPassword(mContext);
				    		dbAdapter = new HostListDbAdapter(mContext);
				    		dbAdapter.open();
				    		dbAdapter.addHost(new ClientId(0, "nativeboinc", "127.0.0.1",
				    				31416, accessPassword));
				    		
				    		// change hostname
				    		NativeBoincUtils.setHostname(mContext, Build.PRODUCT);
				    	} catch(IOException ex) {
				    		notifyError(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
				    				mContext.getString(R.string.getAccessPasswordError));
				    	} finally {
				    		if (dbAdapter != null)
				    			dbAdapter.close();
				    	}
			    	}
			    	
			    	mIsClientBeingInstalled = false;
			    	// starting client
			    	if (!clientToUpdate)
			    		mRunner.startClient(true);
			    }
			    
			    notifyFinish(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
			} catch(Exception ex) {
				if (Logging.ERROR) Log.e(TAG, "on client install finishing:"+
							ex.getClass().getCanonicalName()+":"+ex.getMessage());
				notifyError(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
	    				mContext.getString(R.string.unexpectedError)+": "+ex.getMessage());
			} finally {
				if (Logging.DEBUG) Log.d(TAG, "Release semaphore mDelayedClientShutdownSem");
				mDelayedClientShutdownSem.release();
				// notify that client installer is not working
				
				// unlock all locks for project apps installers
				notifyIfClientUpdatedAndRan();
				notifyIfBenchmarkFinished();
				
				synchronized(this) {
					mClientShouldBeRun = false;
					mIsClientBeingInstalled = false;
					mClientInstaller = null;
					notifyChangeOfIsWorking();
				}
			}
		}
	}
	
	public void installClientAutomatically(boolean standalone) {
		if (mIsClientBeingInstalled)
			return;	// skip
		
		notifyOperation(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
				mContext.getString(R.string.installClientNotifyBegin));
		synchronized(this) {
			mIsClientBeingInstalled = true;
			mClientInstaller = new ClientInstaller(standalone, null);
			// notify that client installer is working
			notifyChangeOfIsWorking();
			mClientInstallerFuture = mExecutorService.submit(mClientInstaller);
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
			boolean directInstallation) {
		/* */
		Thread currentThread = Thread.currentThread();
		
		if (currentThread.isInterrupted())
			return null;
		
		ArrayList<String> fileList = new ArrayList<String>();
		FileOutputStream outStream = null;
		ZipFile zipFile = null;
		InputStream inStream = null;
		
		long totalLength = 0;
		
		StringBuilder projectAppFilePath = new StringBuilder();
		projectAppFilePath.append(mContext.getFilesDir());
		if (directInstallation)
			projectAppFilePath.append("/boinc/projects/");
		else
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
					
					if (currentThread.isInterrupted()) {
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
				projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
				
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
		
		return finalOfPuttingProjectApplications(fileList, projectDistrib,
				projectAppFilePath, projectDirPathLength);
	}
	
	private ArrayList<String> finalOfPuttingProjectApplications(ArrayList<String> fileList,
			ProjectDistrib projectDistrib, StringBuilder projectAppFilePath, int projectDirPathLength) {
		Thread currentThread = Thread.currentThread();
		
		if (currentThread.isInterrupted()) {
			notifyCancel(projectDistrib.projectName, projectDistrib.projectUrl);
			return null;
		}

		FileInputStream inStream = null;
		/* change permissions */
		ArrayList<String> execFilenames = null;
		try {
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
		
		if (currentThread.isInterrupted()) {
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
		
		if (currentThread.isInterrupted()) {
			notifyCancel(projectDistrib.projectName, projectDistrib.projectUrl);
			return null;
		}
		
		return fileList;
	}
	
	private ArrayList<String> copyProjectApplications(ProjectDistrib projectDistrib, String sdCardPath,
			boolean directInstallation) {
		Thread currentThread = Thread.currentThread();
		
		if (currentThread.isInterrupted())
			return null;
		
		ArrayList<String> fileList = new ArrayList<String>();
		
		StringBuilder projectAppFilePath = new StringBuilder();
		projectAppFilePath.append(mContext.getFilesDir());
		if (directInstallation)
			projectAppFilePath.append("/boinc/projects/");
		else
			projectAppFilePath.append("/boinc/updates/");
		
		projectAppFilePath.append(escapeProjectUrl(projectDistrib.projectUrl));
		projectAppFilePath.append("/");
		int projectDirPathLength = projectAppFilePath.length();
		
		File updateDir = new File(projectAppFilePath.toString());
		
		if (!updateDir.isDirectory())
			updateDir.mkdirs();
		
		File sdCardFile = new File(sdCardPath);
		for (File file: sdCardFile.listFiles()) {
			FileInputStream inStream = null;
			FileOutputStream outStream = null;
			
			projectAppFilePath.append(file.getName());
			fileList.add(file.getName());
			
			try {
				inStream = new FileInputStream(file);
				outStream = new FileOutputStream(projectAppFilePath.toString());
				projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
				
				byte[] buffer = new byte[BUFFER_SIZE];
				
				while(true) {
					int readed = inStream.read(buffer);
					if (readed == -1)
						break;
					
					outStream.write(buffer, 0, readed);
				}
			} catch (IOException ex) {
				projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
				
				notifyError(projectDistrib.projectName, projectDistrib.projectUrl,
						mContext.getString(R.string.unpackApplicationError));
				
				projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
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
					if (inStream != null)
						inStream.close();
				} catch(IOException ex) { }
				try {
					if (outStream != null)
						outStream.close();
				} catch(IOException ex) { }
			}
		}
		
		return finalOfPuttingProjectApplications(fileList, projectDistrib,
				projectAppFilePath, projectDirPathLength);
	}
	
	private class ProjectAppsInstaller implements Runnable {
		private String mInput;
		private ProjectDistrib mProjectDistrib;
		private ArrayList<String> mFileList;
		private boolean mFromSDCard = false;
		
		private boolean mIsRan = false;
		
		private Future<?> mFuture = null;
		
		/* if client not installed and ran then acquired */ 
		private Semaphore mClientUpdatedAndRanSem = new Semaphore(1);
		
		/* should be released after benchmark */
		private Semaphore mBenchmarkFinishSem = new Semaphore(1);
		
		/**
		 * 
		 * @param input - path zip or directory or zipFileName
		 * @param projectDistrib
		 * @param fromSDCard
		 */
		public ProjectAppsInstaller(String input, ProjectDistrib projectDistrib,
				boolean fromSDCard) {
			mInput = input;
			mProjectDistrib = projectDistrib;
			mFromSDCard = fromSDCard;
			/* acquiring all semaphores */
			try {
				mClientUpdatedAndRanSem.acquire();
			} catch(InterruptedException ex) { }
			try {
				mBenchmarkFinishSem.acquire();
			} catch(InterruptedException ex) { }
		}
		
		public void setFuture(Future<?> future) {
			mFuture = future;
		}
		
		public Future<?> getFuture() {
			return mFuture;
		}
		
		public ProjectDistrib getProjectDistrib() {
			return mProjectDistrib;
		}
		
		public boolean isRan() {
			return mIsRan;
		}
		
		@Override
		public void run() {
			Thread currentThread = Thread.currentThread();
			try {
				if (Logging.DEBUG) Log.d(TAG, "Runned installer for:"+mProjectDistrib.projectUrl);
				
				mIsRan = true;
				
				if (currentThread.isInterrupted() || (mFuture != null && mFuture.isCancelled())) {
					notifyCancel(mProjectDistrib.projectName, mProjectDistrib.projectUrl);
					return;
				}
				
				/* awaiting for benchmark finish */
				notifyOperation(mProjectDistrib.projectName, mProjectDistrib.projectUrl,
						mContext.getString(R.string.waitingForBenchmarkFinish));
				try {
					mBenchmarkFinishSem.acquire();
				} catch(InterruptedException ex) {
					if (Logging.DEBUG) Log.d(TAG,
							"Cancelled during waiting benchmark finish:"+mProjectDistrib.projectUrl);
					notifyCancel(mProjectDistrib.projectName, mProjectDistrib.projectUrl);
					return;
				} finally {
					mBenchmarkFinishSem.release();
				}
				
				if (Logging.DEBUG) Log.d(TAG,
						"after benchmark finish :"+mProjectDistrib.projectUrl);
				
				String zipUrl = mContext.getString(R.string.installAppsSourceUrl) + mInput;
				/* do download and verify */
				String outZipFilename = null;
				if (!mFromSDCard) {
					outZipFilename = mProjectDistrib.projectName+".zip";
					try {
						mDownloader.downloadFile(zipUrl, outZipFilename,
								mContext.getString(R.string.downloadApplication),
								mContext.getString(R.string.downloadApplicationError), true,
								mProjectDistrib.projectName, mProjectDistrib.projectUrl);
						
						if (currentThread.isInterrupted()) {
							mContext.deleteFile(outZipFilename);
							notifyCancel(mProjectDistrib.projectName, mProjectDistrib.projectUrl);
							return;
						}
						
						int status = mDownloader.verifyFile(mContext.getFileStreamPath(outZipFilename),
								zipUrl, true, mProjectDistrib.projectName, mProjectDistrib.projectUrl);
						
						if (status == Downloader.VERIFICATION_CANCELLED) {
							mContext.deleteFile(outZipFilename);
							notifyCancel(mProjectDistrib.projectName, mProjectDistrib.projectUrl);
							return;	// cancelled
						}
						if (status == Downloader.VERIFICATION_FAILED) {
							notifyError(mProjectDistrib.projectName, mProjectDistrib.projectUrl,
									mContext.getString(R.string.verifySignatureFailed));
							return;	// cancelled
						}
					} catch(InstallationException ex) {
						mContext.deleteFile(outZipFilename);
						return;
					}
				}
				
				if (currentThread.isInterrupted()) {
					if (outZipFilename != null)
						mContext.deleteFile(outZipFilename);
					notifyCancel(mProjectDistrib.projectName, mProjectDistrib.projectUrl);
					return;
				}
				
				if (Logging.DEBUG) Log.d(TAG, "After unpacking");
				
				// determine whether direct installation or not
				boolean directInstallation = !mRunner.isRun() && !mClientShouldBeRun;
				
				/* do install in project directory */
				if (!mFromSDCard) {
					mFileList = unpackProjectApplications(mProjectDistrib,
							mContext.getFileStreamPath(outZipFilename).getAbsolutePath(), directInstallation);
					
					if (outZipFilename != null)
						mContext.deleteFile(outZipFilename);
				} else if (mInput.endsWith(".zip")) {
					// if zip from sdcard
					mFileList = unpackProjectApplications(mProjectDistrib, mInput, directInstallation);
					
					if (mFileList == null)
						return;
				
					if (outZipFilename != null)
						mContext.deleteFile(outZipFilename);
				} else {
					// copy to update apps directory
					mFileList = copyProjectApplications(mProjectDistrib, mInput, directInstallation);
					
					if (mFileList == null)
						return;
				}
					
				if (currentThread.isInterrupted()) {
					notifyCancel(mProjectDistrib.projectName, mProjectDistrib.projectUrl);
					return;
				}
				
				if (!directInstallation) {
					if (Logging.DEBUG) Log.d(TAG, "On indirect project installation");
					
					notifyOperation(mProjectDistrib.projectName, mProjectDistrib.projectUrl,
							mContext.getString(R.string.waitingForClientUpdateAndRun));
					
					try {
						mClientUpdatedAndRanSem.acquire();
					} catch(InterruptedException ex) {
						Log.d(TAG, "Cancelled during waiting for client update");
						notifyCancel(mProjectDistrib.projectName, mProjectDistrib.projectUrl);
						return;
					} finally {
						mClientUpdatedAndRanSem.release();
					}
					
					if (mRunner.isRun()) {
						// if really run
						notifyOperation(mProjectDistrib.projectName, mProjectDistrib.projectUrl,
								mContext.getString(R.string.finalizeProjectAppInstall));
						
						/* update in client side */
						if (Logging.DEBUG) Log.d(TAG, "Run update_apps on boinc_client for "+
									mProjectDistrib.projectName);
						mRunner.updateProjectApps(mProjectDistrib.projectUrl);
					} else {
						// otherwise move to proper place
						StringBuilder projectAppFilePath = new StringBuilder();
						StringBuilder updateAppFilePath = new StringBuilder();
						
						if (Logging.DEBUG) Log.d(TAG, "During installation client run is failed!!!");
						
						String escapedUrl = escapeProjectUrl(mProjectDistrib.projectUrl);
						
						updateAppFilePath.append(mContext.getFilesDir());
						updateAppFilePath.append("/boinc/updates/");
						updateAppFilePath.append(escapedUrl);
						updateAppFilePath.append("/");
						
						projectAppFilePath.append(mContext.getFilesDir());
						projectAppFilePath.append("/boinc/projects/");
						projectAppFilePath.append(escapedUrl);
						projectAppFilePath.append("/");
						int projectDirPathLength = projectAppFilePath.length();
						
						if (Logging.DEBUG) Log.d(TAG, "Copy from "+updateAppFilePath+" to "+
									projectAppFilePath);
						
						File updateDirFile = new File(updateAppFilePath.toString());
						// rename files
						for (File file: updateDirFile.listFiles()) {
							projectAppFilePath.append(file.getName());
							file.renameTo(new File(projectAppFilePath.toString()));
							projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
						}
						updatedProjectApps(mProjectDistrib.projectUrl);
					}
				} else {
					// direct installation (finalize)
					updatedProjectApps(mProjectDistrib.projectUrl);
				}
			} catch(Exception ex) {
				if (Logging.ERROR) Log.e(TAG, "on project install finishing:"+
						ex.getClass().getCanonicalName()+":"+ex.getMessage());
				notifyError(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
	    				mContext.getString(R.string.unexpectedError)+": "+ex.getMessage());
			} finally {
				// notify change of is working
				notifyChangeOfIsWorking();
			}
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
		
		// installBoincApplicationAutomatically(zipFilename, projectDistrib);
		// run in separate thread
		final ProjectAppsInstaller appInstaller = new ProjectAppsInstaller(zipFilename,
				projectDistrib, false);
		// unlock lock for client updating
		if (!mIsClientBeingInstalled)
			appInstaller.notifyIfClientUpdatedAndRan();
		
		SharedPreferences sharedPrefs = PreferenceManager
				.getDefaultSharedPreferences(mContext);
		
		boolean awaitForBenchmark = sharedPrefs.getBoolean(PreferenceName.WAITING_FOR_BENCHMARK, false);
		if (Logging.DEBUG) Log.d(TAG, "Await for benchmark:"+awaitForBenchmark);
		
		if (awaitForBenchmark) {
			postDelayed(new Runnable() {
				@Override
				public void run() {
					if (!mBenchmarkIsRun) // if again benchmark is not ran
						// unlock benchmark lock
						appInstaller.notifyIfBenchmarkFinished();
				}
			}, MAX_PERIOD_BEFORE_BENCHMARK);
			
			// unset waiting for benchmark
			sharedPrefs.edit().putBoolean(PreferenceName.WAITING_FOR_BENCHMARK, false)
					.commit();
			
		} else // no wait
			appInstaller.notifyIfBenchmarkFinished();
		
		notifyOperation(projectDistrib.projectName, projectUrl,
				mContext.getString(R.string.installProjectBegin));
		
		synchronized(this) {
			mProjectAppsInstallers.put(projectDistrib.projectUrl, appInstaller);
			notifyChangeOfIsWorking();
			Future<?> future = mExecutorService.submit(appInstaller);
			appInstaller.setFuture(future);
		}
	}
	
	
	public void reinstallUpdateItems(ArrayList<UpdateItem> updateItems) {
		
		/* first, install client */
		for (UpdateItem updateItem: updateItems)
			if (updateItem.name.equals(InstallerService.BOINC_CLIENT_ITEM_NAME))
				/* update boinc client */
				if (!mIsClientBeingInstalled) {
					notifyOperation(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
							mContext.getString(R.string.installClientNotifyBegin));
					synchronized(this) {
						mIsClientBeingInstalled = true;
						notifyChangeOfIsWorking();
						mClientInstaller = new ClientInstaller(false, null);
						mExecutorService.submit(mClientInstaller);
					}
				}
		
		/* next install project applications */
		for (UpdateItem updateItem: updateItems) {
			if (!updateItem.name.equals(InstallerService.BOINC_CLIENT_ITEM_NAME)) {
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
				notifyOperation(foundDistrib.projectName, foundDistrib.projectUrl,
						mContext.getString(R.string.installProjectBegin));
				
				final ProjectAppsInstaller appInstaller =
						new ProjectAppsInstaller(updateItem.filename, foundDistrib, false);

				// no await for benchmark
				appInstaller.notifyIfBenchmarkFinished();
				
				synchronized(this) {
					mProjectAppsInstallers.put(foundDistrib.projectUrl, appInstaller);
					notifyChangeOfIsWorking();
					Future<?> future = mExecutorService.submit(appInstaller);
					appInstaller.setFuture(future);
				}
			}
		}
		
		// unlocks project apps installer if client not installed
		if (!mIsClientBeingInstalled) {
			if (Logging.DEBUG) Log.d(TAG, "if only project apps will being installed");
			notifyIfClientUpdatedAndRan();
		}
	}
	
	public void updateDistribsFromSDCard(String dirPath, String[] distribNames) {
		/* first, install client */
		for (String distribName: distribNames)
			if (distribName.equals(InstallerService.BOINC_CLIENT_ITEM_NAME)) {
				File clientFile = new File(dirPath+"boinc_client");
				
				if (!clientFile.exists()) // if zip
					clientFile = new File(dirPath+"boinc_client.zip");
				
				/* update boinc client */
				if (!mIsClientBeingInstalled) {
					notifyOperation(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
							mContext.getString(R.string.installClientNotifyBegin));
					synchronized(this) {
						mIsClientBeingInstalled = true;
						notifyChangeOfIsWorking();
						mClientInstaller = new ClientInstaller(false, clientFile.getAbsolutePath());
						mExecutorService.submit(mClientInstaller);
					}
				}
			}
		
		/* next install project applications */
		for (String distribName: distribNames) {
			if (!distribName.equals(InstallerService.BOINC_CLIENT_ITEM_NAME)) {
				/* install */
				ProjectDistrib foundDistrib = null;
				for (ProjectDistrib distrib: mProjectDistribs)
					if (distrib.projectName.equals(distribName)) {
						foundDistrib = distrib;
						break;
					}
				if (isProjectAppBeingInstalled(foundDistrib.projectUrl))
					continue;	// if being installed
				
				File distribFile = new File(dirPath+distribName);
				
				if (!distribFile.exists()) // if zip
					distribFile = new File(dirPath+distribName+".zip");
				
				// run in separate thread
				notifyOperation(foundDistrib.projectName, foundDistrib.projectUrl,
						mContext.getString(R.string.installProjectBegin));
				
				final ProjectAppsInstaller appInstaller =
						new ProjectAppsInstaller(distribFile.getAbsolutePath(), foundDistrib, true);

				// no await for benchmark
				appInstaller.notifyIfBenchmarkFinished();
				
				synchronized(this) {
					mProjectAppsInstallers.put(foundDistrib.projectUrl, appInstaller);
					notifyChangeOfIsWorking();
					Future<?> future = mExecutorService.submit(appInstaller);
					appInstaller.setFuture(future);
				}
			}
		}
		
		// unlocks project apps installer if client not installed
		if (!mIsClientBeingInstalled) {
			if (Logging.DEBUG) Log.d(TAG, "if only project apps will being installed");
			notifyIfClientUpdatedAndRan();
		}
	}
	
	private boolean parseProjectDistribs(String filename, boolean notify, boolean notifyResult) {
		InputStream inStream = null;
		try {
			inStream = mContext.openFileInput(filename);;
			
			/* parse and notify */
			mProjectDistribs = ProjectDistribListParser.parse(inStream);
			if (mProjectDistribs != null) {
				if (notifyResult)
					notifyProjectDistribs(new ArrayList<ProjectDistrib>(mProjectDistribs));
				// if success
				return true;
			} else if (notify) // notify error
				notifyError("", "", mContext.getString(R.string.appListParseError));
		} catch(IOException ex) {
			if (notify)
				notifyError("", "", mContext.getString(R.string.appListParseError));
		} finally {
			try {
				if (inStream != null) inStream.close();
			} catch(IOException ex) { }
		}
		// failed
		return false;
	}
	
	/**
	 * @return true if success
	 */
	public boolean updateProjectDistribList(boolean notifyResult) {
		String appListUrl = mContext.getString(R.string.installAppsSourceUrl)+"apps.xml";
		
		synchronized(this) {
			mHandlerIsWorking = true;
			notifyChangeOfIsWorking();
		}
		try {
			mDownloader.downloadFile(appListUrl, "apps.xml",
					mContext.getString(R.string.appListDownload),
					mContext.getString(R.string.appListDownloadError), false, "", "");
			
			int status = mDownloader.verifyFile(mContext.getFileStreamPath("apps.xml"),
					appListUrl, false, "", "");
			
			if (status == Downloader.VERIFICATION_CANCELLED) {
				Thread.interrupted(); // clear interrupted flag
				mContext.deleteFile("apps.xml");
				notifyCancel("", "");
				return false;	// cancelled
			}
			if (status == Downloader.VERIFICATION_FAILED) {
				notifyError("", "", mContext.getString(R.string.verifySignatureFailed));
				mContext.deleteFile("apps.xml");
				/* errors occurred, but we returns previous value */
				if (mProjectDistribs != null)
					notifyProjectDistribs(new ArrayList<ProjectDistrib>(mProjectDistribs));
				return true;
			}
			
			if (Thread.interrupted()) // if cancelled
				return false;
			
			/* parse it */
			if (parseProjectDistribs("apps.xml", true, notifyResult)) {
				// make backup
				mContext.getFileStreamPath("apps.xml").renameTo(
						mContext.getFileStreamPath("apps-old.xml"));			
			}
		} catch(InstallationException ex) {
			mContext.deleteFile("apps.xml");
			/* errors occurred, but we returns previous value */
			if (mProjectDistribs != null)
				notifyProjectDistribs(new ArrayList<ProjectDistrib>(mProjectDistribs));
			return true;
		} finally {
			// hanbdler doesnt working currently
			synchronized(this) {
				mHandlerIsWorking = false;
				notifyChangeOfIsWorking();
			}
		}
		return true;
	}
	
	/* remove detached projects from installed projects */
	public void synchronizeInstalledProjects() {
		mDistribManager.synchronizeWithProjectList(mContext);
	}
	
	public synchronized void cancelAll() {
		cancelClientInstallation();
		cancelDumpFiles();
		for (ProjectAppsInstaller appInstaller: mProjectAppsInstallers.values()) {
			appInstaller.getFuture().cancel(true);
			if (!appInstaller.isRan()) { // notify cancel if not ran yet
				ProjectDistrib distrib = appInstaller.getProjectDistrib();
				notifyCancel(distrib.projectName, distrib.projectUrl);
			}
		}
		mProjectAppsInstallers.clear();
	}
	
	public synchronized void cancelClientInstallation() {
		if (mClientInstallerFuture != null) {
			mClientInstallerFuture.cancel(true);
			if (mClientInstaller != null && !mClientInstaller.isRan()) // notify cancel if not ran yet
				notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
		}
		mClientInstallerFuture = null;
		mClientInstaller = null;
	}
	
	public synchronized void cancelDumpFiles() {
		mInstallOps.cancelDumpFiles();
	}
	
	public synchronized void cancelOperation(Thread workingThread) {
		if (isWorking()) {
			workingThread.interrupt();
		}
			
	}
	
	public synchronized boolean isWorking() {
		return  mIsClientBeingInstalled || !mProjectAppsInstallers.isEmpty() || mHandlerIsWorking;
	}
	
	public synchronized void cancelProjectAppsInstallation(String projectUrl) {
		ProjectAppsInstaller appsInstaller = mProjectAppsInstallers.remove(projectUrl);
		if (appsInstaller != null) {
			appsInstaller.getFuture().cancel(true);
			if (!appsInstaller.isRan()) { // notify cancel if not ran yet
				ProjectDistrib distrib = appsInstaller.getProjectDistrib();
				notifyCancel(distrib.projectName, distrib.projectUrl);
			}
		}
	}
	
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
	
	private boolean mDoFetchBinariesToUpdateOrInstall = false;
	private Semaphore mObtainedProjectsSem = new Semaphore(1);
	private ArrayList<Project> mObtainedProjects = null;
	
	/**
	 * get list of binaries to update or install (from project url)
	 * @return update list (sorted)
	 */
	public void getBinariesToUpdateOrInstall() {
		mHandlerIsWorking = true;
		notifyChangeOfIsWorking();
		synchronizeInstalledProjects();
		if (mRunner.isRun()) {
			// if ran
			mObtainedProjects = null;
			mDoFetchBinariesToUpdateOrInstall = true;
			
			// lock semaphore
			try {
				mObtainedProjectsSem.acquire();
			} catch(InterruptedException ex) { }
			
			// run get projects
			mRunner.getProjects(this);
			
			try {
				mObtainedProjectsSem.tryAcquire(5000, TimeUnit.MILLISECONDS);
			} catch(InterruptedException ex) { 
			} finally {
				// auto releasing of semaphore
				mObtainedProjectsSem.release();
			}
			
			if (Logging.DEBUG) Log.d(TAG, "getBinariesToUpdateOrInstall: projects from client:"+
					(mObtainedProjects != null));
			// go to main routine
			getBinariesToUpdateOrInstallInternal(mObtainedProjects);
		}
		else // if not run
			getBinariesToUpdateOrInstallInternal(null);
	}
	
	public void getBinariesToUpdateOrInstallInternal(ArrayList<Project> fetchedProjects) {
		
		String[] attachedProjectUrls = null;
		
		if (fetchedProjects == null) {
			/* get project list from boinc directory */
			if (Logging.DEBUG) Log.d(TAG, "ProjectUrls From native boinc directory");
			FileInputStream inputStream = null;
			try {
				inputStream = new FileInputStream(
						mContext.getFileStreamPath("boinc").getAbsolutePath()+"/client_state.xml");
				attachedProjectUrls = ProjectUrlsClientStateParser.parse(inputStream);
			} catch(IOException ex) {
				// if error
				notifyError("", "", "Cant");
				return;
			} finally {
				try {
					inputStream.close();
				} catch(IOException ex) { }
			}
		} else {
			if (Logging.DEBUG) Log.d(TAG, "ProjectUrls From native boinc client");
			// prepare project urls list
			attachedProjectUrls = new String[fetchedProjects.size()];
			for (int i = 0; i < attachedProjectUrls.length; i++)
				attachedProjectUrls[i] = fetchedProjects.get(i).master_url;
		}
		
		ArrayList<UpdateItem> updateItems = new ArrayList<UpdateItem>();
		ArrayList<InstalledDistrib> installedDistribs = mDistribManager.getInstalledDistribs();
		
		/* update client and project list */
		if (!updateClientDistrib(false))
			return;
		if (!updateProjectDistribList(false))
			return;
		
		mHandlerIsWorking = true;
		notifyChangeOfIsWorking();
		
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
				VersionUtil.compareVersion(mClientDistrib.version, clientVersion) > 0) {
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
						/* if version newest than installed or distrib from sdcard */
						(installedDistrib.fromSDCard || 
						 VersionUtil.compareVersion(projectDistrib.version, installedDistrib.version) > 0)) {
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
		
		// notify final result
		notifyBinariesToInstallOrUpdate(array);
		
		mHandlerIsWorking = false;
		notifyChangeOfIsWorking();
	}
	
	/**
	 * get list of distrib to update from sdcard
	 */
	public String[] getBinariesToUpdateFromSDCard(String path) {
		File dirFile = new File(path);
		if (!dirFile.isDirectory())
			return null;
		
		File[] fileList = dirFile.listFiles();
		if (fileList == null)
			return null;
		
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
			
			for (InstalledDistrib distrib: installedDistribs)
				if (distrib.projectName.equals(distribName)) {
					distribsToUpdate.add(distribName);
					break;
				}
		}
		return distribsToUpdate.toArray(new String[0]);
	}
	
	/**
	 * dump boinc files (installation) to directory
	 */
	public void dumpBoincFiles(String directory) {
		if (mDoDumpBoincFiles)
			return;
		
		
		mDoDumpBoincFiles = true;
		mHandlerIsWorking = true;
		notifyChangeOfIsWorking();
		
		mInstallOps.dumpBoincFiles(directory);
		
		mHandlerIsWorking = false;
		notifyChangeOfIsWorking();
		
		mDoDumpBoincFiles = false;
	}
	
	/**
	 * reinstall boinc (delete directory, and make first start)
	 */
	public void reinstallBoinc() {
		if (mDoBoincReinstall)
			return;
		
		mDoBoincReinstall = true;
		mHandlerIsWorking = true;
		notifyChangeOfIsWorking();
		
		mInstallOps.reinstallBoinc();
		
		mHandlerIsWorking = false;
		notifyChangeOfIsWorking();
		mDoBoincReinstall = false;
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
	
	private synchronized void notifyBinariesToInstallOrUpdate(final UpdateItem[] updateItems) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.notifyBinariesToInstallOrUpdate(updateItems);
			}
		});
	}
	
	private synchronized void notifyChangeOfIsWorking() {
		final boolean currentIsWorking = isWorking();
		if (mPreviousStateOfIsWorking != currentIsWorking) {
			mPreviousStateOfIsWorking = currentIsWorking;
			mListenerHandler.post(new Runnable() {
				@Override
				public void run() {
					mListenerHandler.onChangeIsWorking(currentIsWorking);
				}
			});
		}
	}

	@Override
	public void onNativeBoincClientError(String message) {
	}

	@Override
	public synchronized void updatedProjectApps(String projectUrl) {
		ProjectAppsInstaller appsInstaller = mProjectAppsInstallers.remove(projectUrl);
		if (Logging.DEBUG) Log.d(TAG, "After update on client side: "+
				appsInstaller.mProjectDistrib.projectName);
		
		if (!appsInstaller.mFromSDCard)
			mDistribManager.addOrUpdateDistrib(appsInstaller.mProjectDistrib, appsInstaller.mFileList);
		else // if from sdcard
			mDistribManager.addOrUpdateDistribFromSDCard(appsInstaller.mProjectDistrib.projectName,
					appsInstaller.mProjectDistrib.projectUrl, appsInstaller.mFileList);
		
		notifyFinish(appsInstaller.mProjectDistrib.projectName, projectUrl);
		
		notifyChangeOfIsWorking();
	}
	
	@Override
	public synchronized void updateProjectAppsError(String projectUrl) {
		ProjectAppsInstaller appsInstaller = mProjectAppsInstallers.remove(projectUrl);
		if (Logging.DEBUG) Log.d(TAG, "After update on client side error: "+
				appsInstaller.mProjectDistrib.projectName);
		
		notifyError(appsInstaller.mProjectDistrib.projectName, projectUrl,
				mContext.getString(R.string.updateProjectAppError));
		
		mProjectAppsInstallers.remove(projectUrl);
		notifyChangeOfIsWorking();
	}

	@Override
	public void onMonitorEvent(final ClientEvent event) {
		switch(event.type) {
		case ClientEvent.EVENT_ATTACHED_PROJECT:
			// do nothing
			break;
		case ClientEvent.EVENT_DETACHED_PROJECT:
			// synchronize with installed projects
			post(new Runnable() {
				@Override
				public void run() {
					synchronizeInstalledProjects();
				}
			});
			break;
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
	public void onClientStart() {
		if (mIsClientBeingInstalled) {
			mDelayedClientShutdownSem.tryAcquire();
		} else
			notifyIfClientUpdatedAndRan();
	}
	
	@Override
	public void onClientStop(int exitCode, boolean stoppedByManager) {
		if (mIsClientBeingInstalled)
			mDelayedClientShutdownSem.release();
		if (mDoBoincReinstall)
			mInstallOps.notifyShutdownClient();
		
		if (mDoFetchBinariesToUpdateOrInstall) {
			mDoFetchBinariesToUpdateOrInstall = false;
			mObtainedProjects = null;
			mObtainedProjectsSem.release();
		}
	}

	@Override
	public void onNativeBoincServiceError(String message) {
		// also notify
		notifyIfClientUpdatedAndRan();
		if (mDoFetchBinariesToUpdateOrInstall) {
			mDoFetchBinariesToUpdateOrInstall = false;
			mObtainedProjects = null;
			mObtainedProjectsSem.release();
		}
	}

	@Override
	public void onChangeRunnerIsWorking(boolean isWorking) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void getProjects(ArrayList<Project> projects) {
		if (mDoFetchBinariesToUpdateOrInstall) {
			mDoFetchBinariesToUpdateOrInstall = false;
			mObtainedProjects = projects;
			mObtainedProjectsSem.release();
		}
	}
}
