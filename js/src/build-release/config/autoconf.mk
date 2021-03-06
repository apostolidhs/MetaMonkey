include $(DEPTH)/config/emptyvars.mk
ACDEFINES = -DMOZILLA_VERSION=\"24.2.0\" -DMOZILLA_VERSION_U=24.2.0 -DMOZILLA_UAVERSION=\"24.0\" -DMOZJS_MAJOR_VERSION=24 -DMOZJS_MINOR_VERSION=2 -D_CRT_SECURE_NO_WARNINGS=1 -D_CRT_NONSTDC_NO_WARNINGS=1 -DMOZILLA_VERSION=\"24.2.0\" -DMOZILLA_VERSION_U=24.2.0 -DMOZILLA_UAVERSION=\"24.0\" -DMOZJS_MAJOR_VERSION=24 -DMOZJS_MINOR_VERSION=2 -D_CRT_SECURE_NO_WARNINGS=1 -D_CRT_NONSTDC_NO_WARNINGS=1 -DMOZILLA_VERSION=\"24.2.0\" -DMOZILLA_VERSION_U=24.2.0 -DMOZILLA_UAVERSION=\"24.0\" -DMOZJS_MAJOR_VERSION=24 -DMOZJS_MINOR_VERSION=2 -D_CRT_SECURE_NO_WARNINGS=1 -D_CRT_NONSTDC_NO_WARNINGS=1 -DMOZILLA_VERSION=\"24.2.0\" -DMOZILLA_VERSION_U=24.2.0 -DMOZILLA_UAVERSION=\"24.0\" -DMOZJS_MAJOR_VERSION=24 -DMOZJS_MINOR_VERSION=2 -D_CRT_SECURE_NO_WARNINGS=1 -D_CRT_NONSTDC_NO_WARNINGS=1 -DHAVE_WINSDKVER_H=1 -DWINVER=0x502 -D_WIN32_WINNT=0x502 -D_WIN32_IE=0x0603 -DMOZ_WINSDK_TARGETVER=0x06010000 -DMOZ_NTDDI_WIN7=0x06010000 -DHAVE_LOCALECONV=1 -DX_DISPLAY_MISSING=1 -DHAVE_SNPRINTF=1 -D_WINDOWS=1 -DWIN32=1 -DXP_WIN=1 -DXP_WIN32=1 -DHW_THREADS=1 -DSTDC_HEADERS=1 -DNEW_H=\<new\> -DWIN32_LEAN_AND_MEAN=1 -DNO_X11=1 -D_X86_=1 -DD_INO=d_ino -DJS_CPU_X86=1 -DJS_NUNBOX32=1 -DJS_METHODJIT=1 -DJS_MONOIC=1 -DJS_POLYIC=1 -DJS_METHODJIT_TYPED_ARRAY=1 -DJS_ION=1 -DENABLE_YARR_JIT=1 -DNS_ATTR_MALLOC= -DNS_WARN_UNUSED_RESULT= -DMALLOC_H=\<malloc.h\> -DHAVE_FORCEINLINE=1 -DHAVE_LOCALECONV=1 -DMOZ_UPDATE_CHANNEL=default -DRELEASE_BUILD=1 -DMOZ_DEBUG_SYMBOLS=1 -DJSGC_INCREMENTAL=1 -DJS_DEFAULT_JITREPORT_GRANULARITY=3 -DCPP_THROW_NEW=throw\(\) -DMOZ_DLL_SUFFIX=\".dll\" -DU_USING_ICU_NAMESPACE=0
AR = lib
AR_DELETE = $(AR) d
AR_FLAGS = -NOLOGO -OUT:"$@"
AR_LIST = $(AR) t
AS = ml.exe
ASM_SUFFIX = asm
AS_DASH_C_FLAG = -c
AUTOCONF = C:/mozilla-build/msys/bin/autoconf
AWK = gawk
BIN_SUFFIX = .exe
CC = cl
CCACHE = no
CC_VERSION = 18.00.21005.1
CFLAGS = -TC -nologo -W3 -Gy -Fd$(COMPILE_PDBFILE) -wd4244 -we4553
CL_INCLUDES_PREFIX = Note: including file:
CPP = cl -E -nologo
CPU_ARCH = x86
CROSS_LIB = /usr/i686-pc-mingw32
CXX = cl
CXXCPP = cl -TP -E -nologo
CXXFLAGS = -wd4099 -TP -nologo -wd4345 -wd4351 -wd4800 -D_CRT_SECURE_NO_WARNINGS -W3 -Gy -Fd$(COMPILE_PDBFILE) -wd4244 -wd4251 -we4553 -GR-
CXX_VERSION = 18.00.21005.1
DLL_SUFFIX = .dll
DOXYGEN = :
DSO_LDOPTS = -SUBSYSTEM:WINDOWS -MACHINE:X86
ENABLE_ION = 1
ENABLE_METHODJIT = 1
ENABLE_TESTS = 1
ENABLE_YARR_JIT = 1
EXPAND_LIBS_LIST_STYLE = list
GMAKE = C:/mozilla-build/msys/local/bin/make.exe
GRE_MILESTONE = 24.2.0
HOST_AR = lib
HOST_AR_FLAGS = -NOLOGO -OUT:"$@"
HOST_BIN_SUFFIX = .exe
HOST_CC = $(CC)
HOST_CFLAGS = $(CFLAGS) -TC -nologo -Fd$(HOST_PDBFILE) -DXP_WIN32 -DXP_WIN -DWIN32 -D_WIN32 -DNO_X11 -D_CRT_SECURE_NO_WARNINGS
HOST_CXX = $(CXX)
HOST_CXXFLAGS = $(CXXFLAGS)
HOST_LD = $(LD)
HOST_LDFLAGS =   -MACHINE:X86
HOST_NSPR_MDCPUCFG = \"md/_winnt.cfg\"
HOST_OPTIMIZE_FLAGS = -O2
HOST_OS_ARCH = WINNT
HOST_RANLIB = echo ranlib
IMPORT_LIB_SUFFIX = lib
INCREMENTAL_LINKER = 1
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_PROGRAM = ${INSTALL}
INSTALL_SCRIPT = ${INSTALL_PROGRAM}
INTEL_ARCHITECTURE = 1
JS_CONFIG_NAME = js24-config
JS_SHARED_LIBRARY = 1
JS_SHELL_NAME = js24
JS_STANDALONE = 1
LD = link
LDFLAGS =  -LARGEADDRESSAWARE -NXCOMPAT -RELEASE -DYNAMICBASE -SAFESEH
LIBS =  kernel32.lib user32.lib gdi32.lib winmm.lib wsock32.lib advapi32.lib psapi.lib
LIBS_DESC_SUFFIX = desc
LIB_SUFFIX = lib
LN_S = ln
MC = mc.exe
MKCSHLIB = $(LD) -NOLOGO -DLL -OUT:$@ -PDB:$(LINK_PDBFILE) $(DSO_LDOPTS)
MKSHLIB = $(LD) -NOLOGO -DLL -OUT:$@ -PDB:$(LINK_PDBFILE) $(DSO_LDOPTS)
MOZILLA_SYMBOLVERSION = 24
MOZILLA_VERSION = 24.2.0
MOZJS_MAJOR_VERSION = 24
MOZJS_MINOR_VERSION = 2
MOZJS_PATCH_VERSION = 0
MOZ_BUILD_ROOT = f:/japostol/projects/mozjs-24.2.0/js/src/build-release
MOZ_COMPONENT_NSPR_LIBS = $(NSPR_LIBS)
MOZ_DEBUG_DISABLE_DEFS = -DNDEBUG -DTRIMMED
MOZ_DEBUG_ENABLE_DEFS = -DDEBUG -D_DEBUG -DTRACING
MOZ_DEBUG_FLAGS = -Zi
MOZ_DEBUG_LDFLAGS = -DEBUG -DEBUGTYPE:CV
MOZ_DEBUG_SYMBOLS = 1
MOZ_FRAMEPTR_FLAGS = -Oy
MOZ_OPTIMIZE = 1
MOZ_OPTIMIZE_FLAGS = -O2
MOZ_OS2_HIGH_MEMORY = 1
MOZ_TOOLS_DIR = c:/mozilla-build/moztools
MOZ_UPDATE_CHANNEL = default
MSMANIFEST_TOOL = 1
NSPR_PKGCONF_CHECK = nspr
OBJ_SUFFIX = obj
OS_ARCH = WINNT
OS_CFLAGS = -TC -nologo -W3 -Gy -Fd$(COMPILE_PDBFILE) -wd4244 -we4553
OS_COMPILE_CFLAGS = -FI $(DEPTH)/js-confdefs.h -DMOZILLA_CLIENT
OS_COMPILE_CXXFLAGS = -FI $(DEPTH)/js-confdefs.h -DMOZILLA_CLIENT
OS_CXXFLAGS = -wd4099 -TP -nologo -wd4345 -wd4351 -wd4800 -D_CRT_SECURE_NO_WARNINGS -W3 -Gy -Fd$(COMPILE_PDBFILE) -wd4244 -wd4251 -we4553 -GR-
OS_LDFLAGS =  -LARGEADDRESSAWARE -NXCOMPAT -RELEASE -DYNAMICBASE -SAFESEH
OS_LIBS =  kernel32.lib user32.lib gdi32.lib winmm.lib wsock32.lib advapi32.lib psapi.lib
OS_RELEASE = 6.1
OS_TARGET = WINNT
OS_TEST = i686
PERL = /bin/sh /f/japostol/projects/mozjs-24.2.0/js/src/build/msys-perl-wrapper
PKG_SKIP_STRIP = 1
PROFILE_GEN_CFLAGS = -GL
PROFILE_GEN_LDFLAGS = -LTCG:PGINSTRUMENT
PROFILE_USE_CFLAGS = -GL -wd4624 -wd4952
PROFILE_USE_LDFLAGS = -LTCG:PGUPDATE
PYTHON = f:/japostol/projects/mozjs-24.2.0/js/src/build-release/_virtualenv/Scripts/python.exe
RANLIB = echo not_ranlib
RC = rc.exe
RELEASE_BUILD = 1
SHELL = /bin/sh
STRIP = echo not_strip
TAR = tar
TARGET_CPU = i686
TARGET_MD_ARCH = win32
TARGET_NSPR_MDCPUCFG = \"md/_win95.cfg\"
TARGET_OS = mingw32
TARGET_VENDOR = pc
TARGET_XPCOM_ABI = x86-msvc
TOP_DIST = dist
USE_DEPENDENT_LIBS = 1
WINDRES = :
WIN_TOP_SRC = f:/japostol/projects/mozjs-24.2.0/js/src
XARGS = xargs
_MSC_VER = 1800
bindir = ${exec_prefix}/bin
build = i686-pc-mingw32
build_alias = i686-pc-mingw32
build_cpu = i686
build_os = mingw32
build_vendor = pc
datadir = ${prefix}/share
exec_prefix = ${prefix}
host = i686-pc-mingw32
host_alias = i686-pc-mingw32
host_cpu = i686
host_os = mingw32
host_vendor = pc
includedir = ${prefix}/include
infodir = ${prefix}/info
libdir = ${exec_prefix}/lib
libexecdir = ${exec_prefix}/libexec
localstatedir = ${prefix}/var
mandir = ${prefix}/man
oldincludedir = /usr/include
prefix = /usr/local
program_transform_name = s,x,x,
sbindir = ${exec_prefix}/sbin
sharedstatedir = ${prefix}/com
sysconfdir = ${prefix}/etc
target = i686-pc-mingw32
target_alias = i686-pc-mingw32
target_cpu = i686
target_os = mingw32
target_vendor = pc
top_srcdir = ..
include $(topsrcdir)/config/baseconfig.mk
