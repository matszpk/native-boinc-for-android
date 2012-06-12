/**
 * 
 */
package sk.boinc.nativeboinc.installer;

/**
 * @author mat
 *
 */
public class InstallOp {
	private static final int OP_PROGRESS_OP = 0;
	private static final int OP_UPDATE_CLIENT_DISTRIB = 1;
	private static final int OP_UPDATE_PROJECT_DISTRIBS = 2;
	private static final int OP_GET_BINARIES_TO_INSTALL = 3;
	private static final int OP_GET_BINARIES_FROM_SDCARD = 4;
	
	public static final InstallOp ProgressOperation = new InstallOp(OP_PROGRESS_OP);
	public static final InstallOp UpdateClientDistrib = new InstallOp(OP_UPDATE_CLIENT_DISTRIB);
	public static final InstallOp UpdateProjectDistribs = new InstallOp(OP_UPDATE_PROJECT_DISTRIBS);
	public static final InstallOp GetBinariesToInstall = new InstallOp(OP_GET_BINARIES_TO_INSTALL);
	public static final InstallOp GetBinariesFromSDCard(String path) {
		// fix path
		if (path.charAt(0) == '/')
			path = path.substring(1);
		if (path.charAt(path.length()-1) == '/')
			path = path.substring(0, path.length()-1);
		return new InstallOp(OP_GET_BINARIES_FROM_SDCARD, path);
	}
	
	private int opCode;
	private String path;
	
	protected InstallOp(int opCode) {
		this.opCode = opCode;
		path = null;
	}
	
	protected InstallOp(int opCode, String path) {
		this.opCode = opCode;
		this.path = path;
	}
	
	@Override
	public boolean equals(Object ob) {
		if (this == null)
			return false;
		if (this == ob)
			return true;
		
		if (ob instanceof InstallOp) {
			InstallOp op = (InstallOp)ob;
			if (this.opCode != op.opCode)
				return false;
			
			if (opCode == OP_GET_BINARIES_FROM_SDCARD && path != null)
				return path.equals(op.path);
			else
				return true;
		}
		return false;
	}
	
	@Override
	public int hashCode() {
		if (opCode == OP_GET_BINARIES_FROM_SDCARD && path != null)
			return opCode ^ path.hashCode();
		return opCode;
	}
	
	@Override
	public String toString() {
		return "["+opCode+","+path+"]";
	}
}
