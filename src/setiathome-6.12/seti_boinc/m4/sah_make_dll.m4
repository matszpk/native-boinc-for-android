AC_DEFUN([SAH_LINK_DLL],[
  case ${host} in
    *darwin*)
        LINK_DLL="${CC} -fPIC -bundle -flat_namespace -undefined suppress"
        ;;
    *solaris*)
    	LINK_DLL="/usr/ccs/bin/ld -G "
	;;
    *)
        LINK_DLL="${CC} -fPIC -shared"
	;;
  esac     
  AC_SUBST(LINK_DLL)
])
