/* 
 * AndroBOINC - BOINC Manager for Android
 * Copyright (C) 2010, Pavol Michalec
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
import java.io.LineNumberReader;
import java.io.Reader;
import java.io.StringReader;
import java.util.Stack;

public class BoincBaseParser implements BoincContentHandler {

	protected StringBuilder mAllBody = null;
	protected int mCurrentElementStart = -1;
	protected int mCurrentElementEnd = -1;

	protected boolean mTrimCharacters = true; // default is trimming
	
	private boolean mFirstTag = true;
	private boolean mHaveBoincReplyTag = false;
	
	private static final class TagElem {
		public String name;
		public int charStartPos;
		
		public TagElem(String name) {
			this.name = name;
		}
		
		public TagElem(String name, int charStartPos) {
			this.name = name;
			this.charStartPos = charStartPos;
		}
		
		public boolean equals(Object ob) {
			if (ob == null)
				return false;
			if (ob == this)
				return false;
			if (ob instanceof TagElem) {
				TagElem tagElem = (TagElem)ob;
				return name.equals(tagElem.name);
			} else if (ob instanceof String) {
				// compare string
				return name.equalsIgnoreCase((String)ob);
			}
			return false;
		}
	}
	
	private static final class LineCharBuferrer extends LineNumberReader {
		public StringBuilder sb = new StringBuilder();
		
		public LineCharBuferrer(Reader reader) {
			super(reader);
		}
		
		@Override
		public int read() throws IOException {
			int readed = super.read();
			if (readed != -1)
				sb.append((char)readed);
			return readed;
		}
		
		@Override
		public void close() throws IOException {
			sb = null;
			super.close();
		}
	}
	
	public final static void parse(BoincBaseParser parser, String istr, boolean checkBoincReplyTag)
			throws BoincParserException {
		StringReader reader = new StringReader(istr);
		parse(parser, reader, checkBoincReplyTag);
	}
	
	public final static String rtrim(String s) {
		int pos;
		for (pos = s.length()-1; pos>=0; pos--)
			if (!Character.isWhitespace(s.charAt(pos)))
				break;
		return s.substring(0, pos+1);
	}
	
	public final static void parse(BoincBaseParser parser, Reader ir, boolean checkBoincReplyTag)
			throws BoincParserException {
		
		parser.mHaveBoincReplyTag = false;
		parser.mFirstTag = true;
		
		LineCharBuferrer lnReader = new LineCharBuferrer(ir);
		try {
			/* parse header of xml (ignores) */
			parser.startDocument();
			Stack<TagElem> tagStack = new Stack<TagElem>();
			int c;
			StringBuilder sb = new StringBuilder();
			// eating characters
			int readedChar = -1;
			
			while(true) {
				if (readedChar == -1)
					c = lnReader.read();
				else {
					c = readedChar;
					readedChar = -1;
				}
				if (c == -1)
					break;
				
				if (c == '<') {
					int tagDelimPos = lnReader.sb.length()-1;
					
					c = lnReader.read();
					if (c == '/') {
						// checking of tag
						sb.setLength(0);
						while((c = lnReader.read()) != -1) {
							if (c == '<') {
								readedChar = c;
								break;
							}
							if (c == '>')
								break;
							sb.append((char)c);
						}
						if (readedChar != -1)
							continue;
						
						if (c == -1)
							throw new BoincParserException(lnReader.getLineNumber()+1,
									"Unexpected end of document");
						
						String endedTagName = rtrim(sb.toString());
						if (tagStack.contains(new TagElem(endedTagName))) {
							// flushing stack
							TagElem tagElem = null;
							boolean foundInStack = false;
							while ((tagElem = tagStack.pop()) != null) {
								if (tagElem.name.equalsIgnoreCase(endedTagName)) {
									foundInStack = true;
									break;
								}
							}
							
							if(!foundInStack)	// ignore it
								continue;
							
							// if tag end
							//String charsString = lnReader.sb.substring(tagElem.charStartPos, tagDelimPos);
							if (parser.mTrimCharacters) {
								StringBuilder allBody = lnReader.sb;
								int startPos = tagElem.charStartPos;
								int endPos = tagDelimPos;
								
								for (; startPos < endPos; startPos++)
									if (!Character.isWhitespace(allBody.charAt(startPos)))
										break;
								
								if (startPos < endPos) {
									for (endPos = endPos-1; endPos >= startPos; endPos--)
										if (!Character.isWhitespace(allBody.charAt(endPos)))
											break;
									endPos++;
								}
								
								parser.characters(lnReader.sb, startPos, endPos);
							} else
								parser.characters(lnReader.sb, tagElem.charStartPos, tagDelimPos);
							parser.endElement(endedTagName);
						}
					} else if (c == -1)
						throw new BoincParserException(lnReader.getLineNumber()+1,
								"Unexpected end of document");
					else {
						// check tag begin
						sb.setLength(0);
						sb.append((char)c);
						while ((c = lnReader.read()) != -1) {
							if (c == '<') {
								readedChar = c;
								break;
							}
							if (c == '>')
								break;
							sb.append((char)c);
						}
						
						if (readedChar != -1)
							continue;
						
						if (c == -1)
							throw new BoincParserException(lnReader.getLineNumber()+1,
									"Unexpected end of document");
						
						String tagName = rtrim(sb.toString());
						boolean endedTag = false;
						if (tagName.endsWith("/")) {
							endedTag = true;
							tagName = rtrim(tagName.substring(0, tagName.length()-1));
						}
						// verify tag name
						boolean verified = true;
						for (int i = 0; i < tagName.length(); i++) {
							c = tagName.charAt(i);
							if (!Character.isLetterOrDigit(c) && c != '_') {
								verified = false;
								break;
							}
						}
						
						if (!verified) // ignore if not verified tag name
							continue;
						
						parser.startElement(tagName);
						if (!endedTag)
							tagStack.push(new TagElem(tagName, lnReader.sb.length()));
						else {
							parser.characters(lnReader.sb, 0, 0);
							parser.endElement(tagName);
						}
					}
				}
			}
			if (!tagStack.isEmpty())
				throw new BoincParserException(lnReader.getLineNumber()+1,
						"Unexpected end of document");
			parser.endDocument();
			
			if (checkBoincReplyTag && !parser.mHaveBoincReplyTag)
				throw new BoincParserException(lnReader.getLineNumber()+1,
						"Reply dont have <boinc_gui_rpc_reply> tag.");
		} catch(IOException ex) {
			throw new BoincParserException(ex);
		} finally {
			try {
				lnReader.close();
			} catch (IOException ex) { }
		}
	}
	
	public String getCurrentElement() {
		return mAllBody.substring(mCurrentElementStart, mCurrentElementEnd);
	}
	
	@Override
	public void characters(StringBuilder chars, int startPos, int endPos) {
		//mCurrentElement = chars.substring(startPos, endPos);
		mAllBody = chars;
		mCurrentElementStart = startPos;
		mCurrentElementEnd = endPos;
	}
	
	@Override
	public void startDocument() {
		
	}

	@Override
	public void endDocument() {
		
	}

	@Override
	public void startElement(String qName) {
		if (mFirstTag) {
			if (qName.equals("boinc_gui_rpc_reply"))
				mHaveBoincReplyTag = true;
			mFirstTag = false;
		}
	}

	@Override
	public void endElement(String qName) {
		
	}
}
