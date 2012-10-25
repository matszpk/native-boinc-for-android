/**
 * 
 */
package edu.berkeley.boinc.lite;

import sk.boinc.nativeboinc.debug.Logging;
import android.util.Log;

/**
 * @author mat
 *
 */
public class ProxyInfoParser extends BoincBaseParser {
	private static final String TAG = "ProxyInfoParser";

	private ProxyInfo mProxyInfo = null;


	public final ProxyInfo getProxyInfo() {
		return mProxyInfo;
	}

	/**
	 * Parse the RPC result (host_info) and generate vector of projects info
	 * @param rpcResult String returned by RPC call of core client
	 * @return HostInfo
	 */
	public static ProxyInfo parse(String rpcResult) {
		try {
			ProxyInfoParser parser = new ProxyInfoParser();
			BoincBaseParser.parse(parser, rpcResult, true);
			return parser.getProxyInfo();
		}
		catch (BoincParserException e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + rpcResult);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return null;
		}		
	}
	
	@Override
	public void startElement(String localName) {
		super.startElement(localName);
		if (localName.equalsIgnoreCase("proxy_info")) {
			if (Logging.INFO) { 
				if (mProxyInfo != null) {
					// previous <host_info> not closed - dropping it!
					Log.i(TAG, "Dropping unfinished <proxy_info> data");
				}
			}
			mProxyInfo = new ProxyInfo();
		}
	}
	
	@Override
	public void endElement(String localName) {
		super.endElement(localName);
		try {
			if (mProxyInfo != null) {
				// we are inside <proxy_info>
				if (localName.equalsIgnoreCase("proxy_info")) {
					// Closing tag of <proxy_info> - nothing to do at the moment
				} else {
					// Not the closing tag - we decode possible inner tags
					if (localName.equalsIgnoreCase("use_http_proxy")) {
						mProxyInfo.use_http_proxy = !getCurrentElement().equals("0");
					} else if (localName.equalsIgnoreCase("use_socks_proxy")) {
						mProxyInfo.use_socks_proxy = !getCurrentElement().equals("0");
					} else if (localName.equalsIgnoreCase("use_http_auth")) {
						mProxyInfo.use_http_auth = !getCurrentElement().equals("0");
					} else if (localName.equalsIgnoreCase("socks_server_name")) {
						mProxyInfo.socks_server_name = getCurrentElement();
					} else if (localName.equalsIgnoreCase("socks_server_port")) {
						mProxyInfo.socks_server_port = Integer.parseInt(getCurrentElement());
					} else if (localName.equalsIgnoreCase("http_server_name")) {
						mProxyInfo.http_server_name = getCurrentElement();
					} else if (localName.equalsIgnoreCase("http_server_port")) {
						mProxyInfo.http_server_port = Integer.parseInt(getCurrentElement());
					} else if (localName.equalsIgnoreCase("socks5_user_name")) {
						mProxyInfo.socks5_user_name = getCurrentElement();
					} else if (localName.equalsIgnoreCase("socks5_user_passwd")) {
						mProxyInfo.socks5_user_passwd = getCurrentElement();
					} else if (localName.equalsIgnoreCase("http_user_name")) {
						mProxyInfo.http_user_name = getCurrentElement();
					} else if (localName.equalsIgnoreCase("http_user_passwd")) {
						mProxyInfo.http_user_passwd = getCurrentElement();
					} else if (localName.equalsIgnoreCase("no_proxy")) {
						mProxyInfo.noproxy_hosts = getCurrentElement();
					}
				}
			}
		} catch(NumberFormatException ex) {
			if (Logging.INFO) Log.i(TAG, "Exception when decoding " + localName);
		}
	}		
}
