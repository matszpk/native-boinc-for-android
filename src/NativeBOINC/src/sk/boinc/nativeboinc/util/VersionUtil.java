/**
 * 
 */
package sk.boinc.nativeboinc.util;

/**
 * @author mat
 *
 */
public class VersionUtil {
	/**
	 * compares this version with specified version and returns -1, 0, -1 for less, equal, greater
	 * @param version1
	 * @param version2
	 * @return -1 for less, 0 for equal, 1 for greater this version 
	 */
	public static int compareVersion(String version1, String version2) {
		int number1 = 0;
		int number2 = 0;
		int i1 = 0, i2 = 0;
		
		int length1 = version1.length();
		int length2 = version2.length();
		
		while (true) {
			if (i1 == length1 && i2 == length2)
				break;	// if equal
			if (i1 == length1)
				return -1;
			if (i2 == length2)
				return 1;
				
			char c1 = version1.charAt(i1);
			char c2 = version2.charAt(i2);
			
			if (Character.isDigit(c1) && Character.isDigit(c2)) {
				int j1 = i1;
				int j2 = i2;
				
				for (; j1 < length1; j1++)
					if (!Character.isDigit(version1.charAt(j1)))
						break;
				
				for (; j2 < length2; j2++)
					if (!Character.isDigit(version2.charAt(j2)))
						break;
				
				number1 = Integer.parseInt(version1.substring(i1, j1));
				number2 = Integer.parseInt(version2.substring(i2, j2));
				/* compare two numbers */
				if (number1 != number2)
					return (number1<number2) ? -1 : 1;
				
				i1 = j1;
				i2 = j2;
			} else {
				if (c1 != c2)
					return (c1<c2) ? -1 : 1;
				i1++;
				i2++;
			}
		}
		return 0;
	}
}
