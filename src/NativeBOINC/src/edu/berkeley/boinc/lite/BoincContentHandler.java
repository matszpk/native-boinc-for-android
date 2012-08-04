/**
 * 
 */
package edu.berkeley.boinc.lite;

/**
 * @author mat
 *
 */
public interface BoincContentHandler {
	public abstract void characters(StringBuilder chars, int startPos, int endPos);
	public abstract void startDocument();
	public abstract void endDocument();
	public abstract void startElement(String localName);
	public abstract void endElement(String localName);
}
