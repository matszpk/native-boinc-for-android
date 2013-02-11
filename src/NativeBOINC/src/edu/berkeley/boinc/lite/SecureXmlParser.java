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
package edu.berkeley.boinc.lite;

import java.io.IOException;
import java.io.InputStream;

import org.xml.sax.ContentHandler;
import org.xml.sax.SAXException;

import android.util.Xml;

/**
 * @author mat
 *
 */
public class SecureXmlParser {
	
	/* wrapper on InputStream, catch IOException and handles it later */
	private static class SecureInputStream extends InputStream {
		private InputStream in;
		private IOException ioEx = null;
		
		public SecureInputStream(InputStream in) {
			this.in = in;
		}
		
		@Override
		public int available() throws IOException {
			try {
				return in.available();
			} catch(IOException ex) {
				ioEx = ex;
				return -1;
			}
		}
		
		@Override
		public void mark(int readLimit) {
			in.mark(readLimit);
		}
		
		@Override
		public boolean markSupported() {
			return in.markSupported();
		}
		
		@Override
		public int read(byte[] buffer) throws IOException {
			try {
				return in.read(buffer);
			} catch(IOException ex) {
				ioEx = ex;
				return -1;
			}
		}
		
		@Override
		public int read(byte[] buffer, int offset, int length) throws IOException {
			try {
				return in.read(buffer, offset, length);
			} catch(IOException ex) {
				ioEx = ex;
				return -1;
			}
		}
		
		@Override
		public int read() throws IOException {
			try {
				return in.read();
			} catch(IOException ex) {
				ioEx = ex;
				return -1;
			}
		}
		
		@Override
		public void reset() throws IOException {
			try {
				in.reset();
			} catch(IOException ex) {
				ioEx = ex;
			}
		}
		
		@Override
		public long skip(long skipCount) throws IOException {
			try {
				return in.skip(skipCount);
			} catch(IOException ex) {
				ioEx = ex;
				return 0;
			}
		}
		
		public void throwIOException() throws IOException {
			if (ioEx != null)
				throw ioEx;
		}
		
		@Override
		public void close() throws IOException {
			try {
				in.close();
			} catch(IOException ex) {
				ioEx = ex;
			}
		}
	}
	
	
	/* prevents Assert error during downloading */
	public static void parse(InputStream inStream, Xml.Encoding encoding, ContentHandler contentHandler)
            throws IOException, SAXException {
		SecureInputStream secIn = new SecureInputStream(inStream);
		Xml.parse(secIn ,encoding, contentHandler);
		secIn.throwIOException();
    }
}
