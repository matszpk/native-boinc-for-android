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


package edu.berkeley.boinc.nativeboinc;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStreamWriter;
import java.io.Writer;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.UnknownHostException;

import android.util.Log;

import sk.boinc.nativeboinc.debug.Logging;

/**
 * @author mat
 *
 */
public class ClientMonitor {
	private static final String TAG = "ClientMonitor";
	
	private static final int CONNECT_TIMEOUT = 1000;
	
	private Socket mSocket;
	private InputStream mInput;
	private Writer mOutput;
	private boolean mConnected = false;
	
	private static final int MAX_BUFFER_SIZE = 4096;
	
	private int mReadBufferPos = 0;
	private int mReadBufferReaded = 0;
	private byte[] mReadBuffer = null;
	private StringBuilder mReply;
	
	public ClientMonitor() { }
	
	public boolean open(String clientUrl, int port, String authCode) {
		if (mConnected) {
			if (Logging.ERROR) Log.e(TAG, "Client monitor already connected");
			close();
			return false;
		}
		if (Logging.DEBUG) Log.d(TAG, "Connecting with client monitor");
		
		try {
			mSocket = new Socket();
			mSocket.connect(new InetSocketAddress(clientUrl, port), CONNECT_TIMEOUT);
			mInput = mSocket.getInputStream();
			mOutput = new OutputStreamWriter(mSocket.getOutputStream(), "ISO-8859-1");
		} catch(UnknownHostException ex) {
			if (Logging.ERROR)
				Log.e(TAG, "Cant connect with client monitor - " + clientUrl + " Unknown host");
			close();
			return false;
		} catch(IOException ex) {
			if (Logging.ERROR) Log.e(TAG, "Cant connect with client monitor:"+ex.getMessage());
			close();
			return false;
		}
		
		if (Logging.DEBUG) Log.d(TAG, "Connected with client monitor. Now authorizing.");
		
		mReadBufferPos = 0;
		mReadBufferReaded = 0;
		mReadBuffer = new byte[MAX_BUFFER_SIZE];
		mReply = new StringBuilder();
		mConnected = true;
		
		if (Logging.INFO) Log.i(TAG, "Authorizing access to monitor");
		boolean success;
		try {
			mOutput.write("<authorize>"+authCode+"</authorize>\n");
			mOutput.flush();
			
			int readed = mInput.read(mReadBuffer);
			if (readed != -1) {
				mReply.append(new String(mReadBuffer, 0, readed));
				if (mReadBuffer[readed-1] == '\003') {
					mReply.setLength(mReply.length()-1);
				}
				/* if return success */
				success = mReply.toString().equals("<success/>");
			} else
				success = false;
		} catch(IOException ex) {
			if (Logging.ERROR) Log.e(TAG, "Cant authorize access to monitor: "+ ex.getMessage());
			close();
			return false;
		}
		
		if (!success) { // if failed
			if (Logging.ERROR) Log.e(TAG, "Cant authorize access to monitor");
			close();	// close connections
		}
		return success;
	}
	
	public boolean isConnected() {
		return mConnected;
	}
	
	public void close() {
		try {
			if (mInput != null)
				mInput.close();
		} catch(IOException ex) {
			if (Logging.WARNING) Log.w(TAG, "Cant close client monitor socket - input");
		}
		
		try {
			if (mOutput != null)
				mOutput.close();
		} catch(IOException ex) {
			if (Logging.WARNING) Log.w(TAG, "Cant close client monitor socket - output");
		}
		
		try {
			if (mSocket != null)
				mSocket.close();
		} catch(IOException ex) {
			if (Logging.WARNING) Log.w(TAG, "Cant close client monitor socket");
		}
		mSocket = null;
		mConnected = false;
		mReadBuffer = null;
		mReply = null;
	}
	
	public ClientEvent poll() throws IOException {
		mReply.setLength(0);
		
		if (mReadBufferPos < mReadBufferReaded) {
			int readPos = mReadBufferPos;
			for (;readPos < mReadBufferReaded; readPos++)
				if (mReadBuffer[readPos] == '\003')
					break;
			if (readPos != mReadBufferReaded) { // if found
				/* parse reply */
				mReply.append(new String(mReadBuffer, mReadBufferPos, readPos-mReadBufferPos));
				mReadBufferPos = readPos+1;
				return ClientEventParser.parse(mReply.toString());
			} else if (mReadBufferPos != 0) {
				// shift rest of content
				System.arraycopy(mReadBuffer, mReadBufferPos, mReadBuffer, 0, mReadBufferReaded-mReadBufferPos);
				mReadBufferReaded -= mReadBufferPos;
				mReadBufferPos = 0;
			}
		} else { // if out of buffer
			mReadBufferPos = 0;
			mReadBufferReaded = 0;
		}
		
		/* reading reply */
		while (true) {
			int readed = mInput.read(mReadBuffer, mReadBufferReaded, MAX_BUFFER_SIZE-mReadBufferReaded);
			if (readed != -1) {
				/* if last chunk */
				mReadBufferReaded += readed;
			} else // break reading
				return null;
			
			int readPos = mReadBufferPos;
			for (;readPos < mReadBufferReaded; readPos++)
				if (mReadBuffer[readPos] == '\003')
					break;
			if (readPos != mReadBufferReaded) {
				/* parse reply */
				mReply.append(new String(mReadBuffer, mReadBufferPos, readPos-mReadBufferPos));
				mReadBufferPos = readPos+1;
				return ClientEventParser.parse(mReply.toString());
			} else if (mReadBufferReaded == MAX_BUFFER_SIZE) { // not handled (longer than 4 kBytes)
				mReadBufferPos = 0;
				mReadBufferReaded = 0;
				return null;
			} else { // if not found and not buffer overflow: do continue reading
				mReadBufferPos = mReadBufferReaded;
			}
		}
	}
}
