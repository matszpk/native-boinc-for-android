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

import java.io.BufferedInputStream;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.net.URI;
import java.net.URL;
import java.net.URLConnection;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Enumeration;
import java.util.Vector;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.utils.URIUtils;
import org.apache.http.impl.client.DefaultHttpClient;
import org.bouncycastle.openpgp.PGPObjectFactory;
import org.bouncycastle.openpgp.PGPPublicKey;
import org.bouncycastle.openpgp.PGPPublicKeyRingCollection;
import org.bouncycastle.openpgp.PGPSignature;
import org.bouncycastle.openpgp.PGPSignatureList;
import org.bouncycastle.openpgp.PGPUtil;

import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.util.Chmod;
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
public class InstallerHandler extends Handler {
	private final static String TAG = "InstallHandler";

	private Context mContext = null;
	
	private InstallerService.ListenerHandler mListenerHandler;
	
	private InstalledDistribManager mDistribManager = null;
	private Vector<ProjectDistrib> mProjectDistribs = null;
	private ClientDistrib mClientDistrib = null;
	
	private static final int NOTIFY_PERIOD = 200; /* milliseconds */
	private static final int BUFFER_SIZE = 4096;
	
	private byte[] pgpKeyContent = null;
	
	private boolean mOperationCancelled = false;
	
	public InstallerHandler(final Context context, InstallerService.ListenerHandler listenerHandler) {
		mContext = context;
		mListenerHandler = listenerHandler;
		mDistribManager = new InstalledDistribManager(context);
		mDistribManager.load();
	}
	
	private void downloadPGPKey() throws InstallationException {
		notifyOperation(mContext.getString(R.string.downloadPGPKey));
		
		DefaultHttpClient client = new DefaultHttpClient();
		
		Reader reader = null;
		FileOutputStream pgpStream = null;
		
		String[] pgpKeyservers = mContext.getResources().getStringArray(R.array.PGPkeyservers); 
		
		boolean isDownloaded = false;
		
		for (String keyserver: pgpKeyservers) {
			if (Logging.DEBUG) Log.d(TAG, "Use PGP keyserver " + keyserver);
			
			if (mOperationCancelled)
				return;
			
			try {
				URI uri = URIUtils.createURI("http", keyserver, 11371, "/pks/lookup",
						"op=get&search=0x"+mContext.getString(R.string.PGPKey), null);
				HttpResponse response = client.execute(new HttpGet(uri));
				
				StringBuilder keyBlock = new StringBuilder();
				
				if (response.getStatusLine().getStatusCode() == HttpStatus.SC_OK) {
					HttpEntity entity = response.getEntity();
					reader = new InputStreamReader(entity.getContent());
					
					char[] buffer = new char[BUFFER_SIZE];
					while (true) {
						int readed = reader.read(buffer);
						if (readed == -1)	// if end
							break;
						keyBlock.append(buffer, 0, readed);
					}
				}
				
				int keyStart = keyBlock.indexOf("-----BEGIN PGP PUBLIC KEY BLOCK-----");
				int keyEnd = keyBlock.indexOf("-----END PGP PUBLIC KEY BLOCK-----");
				
				if (keyStart == -1 || keyEnd == -1)
					throw new Exception("Error");
	
				pgpStream = mContext.openFileOutput("pgpkey.pgp", Context.MODE_PRIVATE);
				
				pgpKeyContent = keyBlock.substring(keyStart, keyEnd+35).getBytes();
				pgpStream.write(pgpKeyContent);
				
				pgpStream.close();
				
				isDownloaded = true;
				break;
			} catch(Exception ex) {	/* on error */
				mContext.deleteFile("pgpkey.pgp");
			} finally {
				try {
					if (reader != null)
						reader.close();
				} catch(IOException ex) { }
				try {
					if (pgpStream != null)
						pgpStream.close();
				} catch(IOException ex) { }
			}
		}
		
		if (!isDownloaded) {
			notifyError(mContext.getString(R.string.downloadPGPKeyError));
			throw new InstallationException();
		}
	}
	
	private static final int VERIFIED_SUCCESFULLY = 1;
	private static final int VERIFICATION_FAILED = 2;
	private static final int VERIFICATION_CANCELLED = 4;
	
	private int verifyFile(File file, String urlString, boolean withProgress) throws InstallationException {
		if (Logging.DEBUG) Log.d(TAG, "verifying file "+urlString);
		FileInputStream pgpStream = null;
		
		if (mOperationCancelled)	// if cancelled
			return VERIFICATION_CANCELLED;
		
		try {
			if (pgpKeyContent == null) {
				if (mContext.getFileStreamPath("pgpkey.pgp").exists())
					pgpStream = mContext.openFileInput("pgpkey.pgp");
				else	// download from keyserver
					downloadPGPKey();
			}
			
			if (pgpKeyContent == null) {
				byte[] content = new byte[4096];
				int readed = pgpStream.read(content);
				if (readed >= 4096) {
					throw new IOException("File too big");
				}
				
				pgpKeyContent = new byte[readed];
				System.arraycopy(content, 0, pgpKeyContent, 0, readed);
			}
		} catch(IOException ex) {
			notifyError(mContext.getString(R.string.loadPGPKeyError));
			throw new InstallationException();
		} finally {
			try {
				if (pgpStream != null)
					pgpStream.close();
			} catch(IOException ex) { }
		}
		
		if (mOperationCancelled)
			return VERIFICATION_CANCELLED;
		
		byte[] signContent = null;
		InputStream signatureStream = null;
		/* download file signature */
		try {
			URL url = new URL(urlString+".asc");
			URLConnection conn = url.openConnection();
			signatureStream = conn.getInputStream();
			
			byte[] content = new byte[4096];
			int readed = signatureStream.read(content);
			if (readed >= 4096) {
				throw new IOException("File too big");
			}
			
			signContent = new byte[readed];
			System.arraycopy(content, 0, signContent, 0, readed);
		} catch(IOException ex) {
			notifyError(mContext.getString(R.string.downloadSignatureError));
			throw new InstallationException();
		} finally {
			try {
				if (signatureStream != null)
					signatureStream.close();
			} catch(IOException ex) { }
		}
		
		if (mOperationCancelled)
			return VERIFICATION_CANCELLED;
		
		String opDesc = mContext.getString(R.string.verifySignature);
		
		notifyOperation(opDesc);
		
		/* verify file signature */
		InputStream bIStream = new ByteArrayInputStream(signContent);
		InputStream contentStream = null;
		
		try {
			bIStream = PGPUtil.getDecoderStream(bIStream);
			
			PGPObjectFactory pgpFact = new PGPObjectFactory(bIStream);
	        
	        PGPSignatureList pgpSignList = (PGPSignatureList)pgpFact.nextObject();
	        
	        InputStream keyInStream = new ByteArrayInputStream(pgpKeyContent);
	        
	        PGPPublicKeyRingCollection pgpPubRingCollection = 
	        	new PGPPublicKeyRingCollection(PGPUtil.getDecoderStream(keyInStream));
	        
	        contentStream = new BufferedInputStream(new FileInputStream(file));

	        PGPSignature signature = pgpSignList.get(0);
	        PGPPublicKey key = pgpPubRingCollection.getPublicKey(signature.getKeyID());
	        
	        signature.initVerify(key, "BC");

	        long length = file.length();
	        
	        int ch;
	        long readed = 0;
	        long time = System.currentTimeMillis();
	        
	        while ((ch = contentStream.read()) >= 0) {
	            signature.update((byte)ch);
	            readed++;
	            
	            if(mOperationCancelled)	// do cancel
	            	return VERIFICATION_CANCELLED;
	            
	            if (withProgress && (readed & 4095) == 0) {
	            	long newTime = System.currentTimeMillis(); 
	            	if (newTime-time > NOTIFY_PERIOD) {
	            		notifyProgress(opDesc, (int)((double)readed*10000.0/(double)length));
	            		time = newTime;
	            	}
	            }
	        }
	        
	        if (withProgress)
	        	notifyProgress(opDesc, InstallerListener.FINISH_PROGRESS);
	        
	        if (signature.verify())
	        	return VERIFIED_SUCCESFULLY;
	        else
	        	return VERIFICATION_FAILED;
		} catch (Exception ex) {
			notifyError(mContext.getString(R.string.verifySignatureError));
			throw new InstallationException();
		} finally {
			try {
				if (contentStream != null)
					contentStream.close();
			} catch(IOException ex) { }
		}
	}
	
	private void downloadFile(String urlString, String outFilename, final String opDesc,
			final String messageError, final boolean withProgress)
					throws InstallationException {
		if (Logging.DEBUG) Log.d(TAG, "downloading file "+urlString);
		
		InputStream inStream = null;
		FileOutputStream outStream = null;
		
		try {
			URL url = new URL(urlString);
			
			notifyOperation(opDesc);
			
			if (!url.getProtocol().equals("ftp")) {
				/* if http protocol */
				URLConnection urlConn = url.openConnection();
				
				inStream = urlConn.getInputStream();
				outStream = new FileOutputStream(mContext.getFileStreamPath(outFilename));
				
				byte[] buffer = new byte[BUFFER_SIZE];
				int length = urlConn.getContentLength();
				int totalReaded = 0;
				long currentTime = System.currentTimeMillis();
				
				while(true) {
					int readed = inStream.read(buffer);
					if (readed == -1)
						break;
					
					if (mOperationCancelled)
						break;	// if canceled
					
					outStream.write(buffer, 0, readed);
					totalReaded += readed;
					
					if (length != -1 && withProgress) {
						long newTime = System.currentTimeMillis();
						if (newTime-currentTime > NOTIFY_PERIOD) {
							notifyProgress(opDesc, (int)((double)totalReaded*10000.0/(double)length));
							currentTime = newTime;
						}
					}
				}
				
				notifyProgress(opDesc, 10000);
			} else {
				throw new UnsupportedOperationException("Unsupported operation");
			}
		} catch(IOException ex) {
			mContext.deleteFile(outFilename);
			notifyError(messageError);
			throw new InstallationException();
		} finally {
			try {
				if (inStream != null)
					inStream.close();
			} catch (IOException ex) { }
			try {
				if (outStream != null)
					outStream.close();
			} catch (IOException ex) { }
		}
	}
	
	public void updateClientDistribList() {
		mOperationCancelled = false;
		
		String clientListUrl = mContext.getString(R.string.installClientSourceUrl)+"client.xml";
		
		if (mOperationCancelled) {
			notifyCancel();
			return;
		}
		
		mClientDistrib = null;
		
		/* download boinc client's list */
		try {	/* download with ignoring */
			downloadFile(clientListUrl, "client.xml",
					mContext.getString(R.string.clientListDownload), 
					mContext.getString(R.string.clientListDownloadError), false);
			
			if (mOperationCancelled) {
				mContext.deleteFile("client.xml");
				notifyCancel();
				return;
			}
			
			/* if doesnt downloaded */
			if (!mContext.getFileStreamPath("client.xml").exists()) {
				notifyClientDistrib(null);
				return;
			}
			
			int status = verifyFile(mContext.getFileStreamPath("client.xml"),
					clientListUrl, false);
			
			if (status == VERIFICATION_CANCELLED) {
				mContext.deleteFile("client.xml");
				notifyCancel();
				return;	// cancelled
			}
			if (status == VERIFICATION_FAILED)
				notifyError(mContext.getString(R.string.verifySignatureFailed));
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
				notifyError(mContext.getString(R.string.clientListParseError));
		} catch(IOException ex) {
			notifyError(mContext.getString(R.string.clientListParseError));
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
	
	public void installClientAutomatically() {
		mOperationCancelled = false;	// reset cancellation	
		String zipFilename = null;
		
		if (mOperationCancelled) {
			notifyCancel();
			return;
		}
		
		zipFilename = mClientDistrib.filename;
		if (Logging.INFO) Log.i(TAG, "Use zip "+zipFilename);
		
		if (mOperationCancelled) {
			notifyCancel();
			return;
		}
		
		/* download and unpack */
		try {
			String zipUrlString = mContext.getString(R.string.installClientSourceUrl)+zipFilename; 
			downloadFile(zipUrlString, "boinc_client.zip",
					mContext.getString(R.string.downloadNativeClient),
					mContext.getString(R.string.downloadNativeClientError), true);
			
			if (mOperationCancelled) {
				mContext.deleteFile("boinc_client.zip");
				notifyCancel();
				return;
			}
			
			int status = verifyFile(mContext.getFileStreamPath("boinc_client.zip"),
					zipUrlString, true);
			
			if (status == VERIFICATION_CANCELLED) {
				mContext.deleteFile("boinc_client.zip");
				notifyCancel();
				return;	// cancelled
			}
			if (status == VERIFICATION_FAILED)
				notifyError(mContext.getString(R.string.verifySignatureFailed));
		} catch(InstallationException ex) {
			/* remove zip file */
			mContext.deleteFile("boinc_client.zip");
			return;
		}
		
		if (mOperationCancelled) {
			mContext.deleteFile("boinc_client.zip");
			notifyCancel();
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
			outStream = mContext.openFileOutput("boinc_client", Context.MODE_PRIVATE);
			
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
				
				if (mOperationCancelled) {
					mContext.deleteFile("boinc_client");
					mContext.deleteFile("boinc_client.zip");
					notifyCancel();
					return;
				}
				
				outStream.write(buffer, 0, readed);
				long newTime = System.currentTimeMillis();
				
				if (newTime-time > NOTIFY_PERIOD) {
					notifyProgress(opDesc, (int)((double)totalReaded*10000.0/(double)length));
					time = newTime;
				}
			}
			notifyProgress(opDesc, InstallerListener.FINISH_PROGRESS);
		} catch(IOException ex) {
			notifyError(mContext.getString(R.string.unpackNativeClientError));
			mContext.deleteFile("boinc_client");
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
		
		if (mOperationCancelled) {
			mContext.deleteFile("boinc_client.zip");
			mContext.deleteFile("boinc_client");
			notifyCancel();
			return;
		}
		
		/* change permissions */
		if (!Chmod.chmod(mContext.getFileStreamPath("boinc_client").getAbsolutePath(), 0700)) {
			notifyError(mContext.getString(R.string.changePermissionsError));
	    	mContext.deleteFile("boinc_client");
	    	return;
		}
		
	    if (mOperationCancelled) {
			notifyCancel();
			return;
		}
	    
	    /* create boinc directory */
	    new File(mContext.getFilesDir()+"/boinc").mkdir();
	    
	    /* save version number */
	    /*SharedPreferences globalPrefs = PreferenceManager.getDefaultSharedPreferences(mContext);
	    SharedPreferences.Editor editor = globalPrefs.edit();
	    
	    if (mClientDistrib != null) {
	    	//editor.putString(PreferenceName.CLIENT_VERSION, mClientDistrib.version);
	    	//if (Logging.DEBUG) Log.d(TAG, "Save client version:" + mClientDistrib.version);
	    }
	    editor.commit();*/
	    if (mClientDistrib != null) {
	    	mDistribManager.setClient(mClientDistrib);
	    	mDistribManager.save();
	    }
	    
	    notifyFinish();
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
	
	private Vector<String> unpackBoincApplication(ProjectDistrib projectDistrib, String zipPath) {
		/* */
		if (mOperationCancelled) {
			return null;
		}
		
		Vector<String> fileList = new Vector<String>();
		FileOutputStream outStream = null;
		ZipFile zipFile = null;
		InputStream inStream = null;
		
		long totalLength = 0;
		
		StringBuilder projectAppFilePath = new StringBuilder();
		projectAppFilePath.append(mContext.getFilesDir());
		projectAppFilePath.append("/boinc/projects/");
		projectAppFilePath.append(escapeProjectUrl(projectDistrib.projectUrl));
		projectAppFilePath.append("/");
		
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
					
					if (mOperationCancelled) {
						notifyCancel();
						return null;
					}
					
					outStream.write(buffer, 0, readed);
					
					long newTime = System.currentTimeMillis();
					if (newTime-time > NOTIFY_PERIOD) {
						notifyProgress(opDesc, (int)((double)totalReaded*10000.0/(double)totalLength));
						time = newTime;
					}
				}
				
				/* reverts to project dir path */
				projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
				outStream = null;
				i++;
			}
			
		} catch(IOException ex) {
			projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
			
			notifyError(mContext.getString(R.string.unpackApplicationError));
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
		
		if (mOperationCancelled) {
			notifyCancel();
			return null;
		}
		
		/* change permissions */
		Vector<String> execFilenames = null;
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
		
		if (mOperationCancelled) {
			notifyCancel();
			return null;
		}
		
		if (execFilenames != null) {
			for (String filename: execFilenames) {
				projectAppFilePath.append(filename);
				if (!Chmod.chmod(projectAppFilePath.toString(), 0700)) {
					notifyError(mContext.getString(R.string.changePermissionsError));
			    	return null;
				}
				projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
			}
		}
		
		if (mOperationCancelled) {
			notifyCancel();
			return null;
		}
		
		return fileList;
	}
	
	public void installBoincApplicationAutomatically(String projectUrl) {
		if (Logging.DEBUG) Log.d(TAG, "install App "+projectUrl);
		
		mOperationCancelled = false;
		
		ProjectDistrib projectDistrib = null;
		
		if (mOperationCancelled) {
			notifyCancel();
			return;
		}
		
		String zipFilename = null;
		/* search project, and retrieve project url */
		for (ProjectDistrib distrib: mProjectDistribs)
			if (distrib.projectUrl.equals(projectUrl)) {
				zipFilename = distrib.filename;
				projectDistrib = distrib;
				break;
			}
		
		if (zipFilename == null) {
			notifyError(mContext.getString(R.string.projectAppNotFound));
			return;
		}
		
		installBoincApplicationAutomatically(zipFilename, projectDistrib);
	}
	
	/**
	 * installs boinc application automatically
	 * @param projectUrl
	 */
	public void installBoincApplicationAutomatically(String zipFilename, ProjectDistrib projectDistrib) {
		
		if (mOperationCancelled) {
			notifyCancel();
			return;
		}
		
		String zipUrl = mContext.getString(R.string.installAppsSourceUrl) + zipFilename;
		
		/* do download and verify */
		try {
			downloadFile(zipUrl, "project_app.zip",
					mContext.getString(R.string.downloadApplication),
					mContext.getString(R.string.downloadApplicationError), true);
			
			if (mOperationCancelled) {
				mContext.deleteFile("project_app.zip");
				notifyCancel();
				return;
			}
			
			int status = verifyFile(mContext.getFileStreamPath("project_app.zip"),
					zipUrl, true);
			
			if (status == VERIFICATION_CANCELLED) {
				mContext.deleteFile("project_app.zip");
				notifyCancel();
				return;	// cancelled
			}
			if (status == VERIFICATION_FAILED)
				notifyError(mContext.getString(R.string.verifySignatureFailed));
		} catch(InstallationException ex) {
			mContext.deleteFile("project_app.zip");
			return;
		}
		
		if (mOperationCancelled) {
			mContext.deleteFile("project_app.zip");
			notifyCancel();
			return;
		}
		
		/* do install in project directory */
		Vector<String> fileList = unpackBoincApplication(projectDistrib,
				mContext.getFileStreamPath("project_app.zip").getAbsolutePath());
		
		mContext.deleteFile("project_app.zip");
		
		if (mOperationCancelled) {
			notifyCancel();
			return;
		}
		
		/* updaste installed distribs */
		mDistribManager.addOrUpdateDistrib(projectDistrib, fileList);
		
		notifyFinish();
	}
	
	public void reinstallUpdateItem(UpdateItem updateItem) {
		if (updateItem.name.equals("BOINC client")) {
			/* update boinc client */
			installClientAutomatically();
		} else {
			if (!updateItem.isNew) {
				/* uninstall boinc application */
				InstalledDistrib installedDistrib = mDistribManager.getInstalledDistribByName(updateItem.name);
				
				StringBuilder projectAppFilePath = new StringBuilder();
				projectAppFilePath.append(mContext.getFilesDir());
				projectAppFilePath.append("/boinc/projects/");
				projectAppFilePath.append(escapeProjectUrl(installedDistrib.projectUrl));
				projectAppFilePath.append("/");
				
				int projectDirPathLength = projectAppFilePath.length();
				
				for (String fileName: installedDistrib.files) {
					projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
					projectAppFilePath.append(fileName);
					// delete file
					new File(projectAppFilePath.toString()).delete();
				}
			}
			
			/* install */
			ProjectDistrib foundDistrib = null;
			for (ProjectDistrib distrib: mProjectDistribs)
				if (distrib.projectName.equals(updateItem.name)) {
					foundDistrib = distrib;
					break;
				}
			
			installBoincApplicationAutomatically(updateItem.filename, foundDistrib);
		}
	}
	
	public void updateProjectDistribList() {
		mOperationCancelled = false;
		
		String appListUrl = mContext.getString(R.string.installAppsSourceUrl)+"apps.xml";
		
		if (mOperationCancelled) {
			notifyCancel();
			return;
		}
		
		try {
			downloadFile(appListUrl, "apps.xml",
					mContext.getString(R.string.appListDownload),
					mContext.getString(R.string.appListDownloadError), false);
			
			if (mOperationCancelled) {
				mContext.deleteFile("apps.xml");
				notifyCancel();
				return;
			}
			
			int status = verifyFile(mContext.getFileStreamPath("apps.xml"),
					appListUrl, false);
			
			if (status == VERIFICATION_CANCELLED) {
				mContext.deleteFile("apps.xml");
				notifyCancel();
				return;	// cancelled
			}
			if (status == VERIFICATION_FAILED)
				notifyError(mContext.getString(R.string.verifySignatureFailed));
		} catch(InstallationException ex) {
			mContext.deleteFile("apps.xml");
			return;
		}
		
		if (mOperationCancelled) {
			mContext.deleteFile("apps.xml");
			notifyCancel();
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
				notifyError(mContext.getString(R.string.appListParseError));
		} catch(IOException ex) {
			notifyError(mContext.getString(R.string.appListParseError));
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
	
	public boolean isClientInstalled() {
		return mContext.getFileStreamPath("boinc_client").exists() &&
			mContext.getFileStreamPath("boinc").isDirectory();
	}
	
	public void cancelOperation() {
		mOperationCancelled = true;
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
		
		Vector<InstalledDistrib> installedDistribs = mDistribManager.getInstalledDistribs();
		int installedCount = installedDistribs.size();
		
		InstalledBinary[] result = new InstalledBinary[installedDistribs.size()+1];
		
		result[0] = new InstalledBinary("BOINC client", installedClient.version,
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
		Vector<UpdateItem> updateItems = new Vector<UpdateItem>();
		
		Vector<InstalledDistrib> installedDistribs = mDistribManager.getInstalledDistribs();
		
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
			updateItems.add(new UpdateItem("BOINC client", mClientDistrib.version, null,
					mClientDistrib.description, mClientDistrib.changes, false));
			firstUpdateBoincAppsIndex = 1;
		}
		
		/* project binaries */
		for (InstalledDistrib installedDistrib: installedDistribs) {
			ProjectDistrib selected = null;
			for (ProjectDistrib projectDistrib: mProjectDistribs)
				if (projectDistrib.projectUrl.equals(installedDistrib.projectUrl) &&
						/* if version newest than installed */
						VersionUtil.compareVersion(projectDistrib.version, installedDistrib.version) > 0) {
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
	private synchronized void notifyOperation(final String opDesc) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onOperation(opDesc);
			}
		});
	}
	
	private synchronized void notifyProgress(final String opDesc, final int progress) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onOperationProgress(opDesc, progress);
			}
		});
	}
	
	private synchronized void notifyError(final String errorMessage) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onOperationError(errorMessage);
			}
		});
	}
	
	private synchronized void notifyCancel() {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onOperationCancel();
			}
		});
	}
	
	private synchronized void notifyFinish() {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onOperationFinish();
			}
		});
	}
	
	private synchronized void notifyProjectDistribs(final Vector<ProjectDistrib> projectDistribs) {
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
}
