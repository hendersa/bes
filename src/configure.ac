AC_PREREQ([2.60])
AC_INIT([BES], [0.1], [], [bes])
AC_REVISION([$Revision: 1.53 $])

AC_CONFIG_SRCDIR([main.cpp])

AC_CANONICAL_TARGET

AC_PROG_CC
AC_PROG_CXX
AC_LANG([C++])

BESFLGS=""
BESDEFS=""
BESLIBS="-ldl"

# VBAM flags
BESDEFS+="$BESDEF -DFRAMESKIP -DNO_FFMPEG -DFINAL_VERSION -DSDL -DNO_LINK -DC_CORE -DNO_ASM"

AC_DEFUN([AC_BES_COMPILER_FLAG],
[
	AC_MSG_CHECKING([whether g++ accepts $1])

	AC_CACHE_VAL([bes_cv_option_$2],
	[
		OLD_CXXFLAGS="[$]CXXFLAGS"
		CXXFLAGS="[$]OLD_CXXFLAGS $1"

		AC_TRY_RUN(
		[
			int	foo;

			int	main (int argc, char **argv)
			{
				/* The following code triggs gcc:s generation of aline opcodes,
				   which some versions of as does not support. */

				if (argc > 0)
					argc = 0;

				return (argc);
			}
		],
		[bes_cv_option_$2="yes"], [bes_cv_option_$2="no"])
	])

	CXXFLAGS="[$]OLD_CXXFLAGS"

	if test "x[$]bes_cv_option_$2" = "xyes"; then
		BESFLGS="[$]BESFLGS $1"
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
		$3
	fi
])

# *****************************
# *** Execution begins here ***
# *****************************

# Remove -g and -O2 flags manually.

if test "x$CFLAGS" != "x"; then
	CFLAGS="`echo \"$CFLAGS\" | sed -e 's/-g//'`"
	CFLAGS="`echo \"$CFLAGS\" | sed -e 's/-O2//'`"
fi

if test "x$CXXFLAGS" != "x"; then
	CXXFLAGS="`echo \"$CXXFLAGS\" | sed -e 's/-g//'`"
	CXXFLAGS="`echo \"$CXXFLAGS\" | sed -e 's/-O2//'`"
fi

# Test what compiler flags we should use.

AC_ARG_ENABLE([debug],
	[AS_HELP_STRING([--enable-debug],
		[leave debug information in the final binary (default: no)])],
	[], [enable_debug="no"])

if test "x$enable_debug" = "xyes"; then
	AC_BES_COMPILER_FLAG([-g],  [g])
	AC_BES_COMPILER_FLAG([-O0], [o0])
else
	AC_BES_COMPILER_FLAG([-O3], [o3], [
		AC_BES_COMPILER_FLAG([-O2], [o2], [
			AC_BES_COMPILER_FLAG([-O1], [o1])])])
	AC_BES_COMPILER_FLAG([-fomit-frame-pointer], [omit_frame_pointer])
fi

AC_ARG_ENABLE([neon],
	[AS_HELP_STRING([--enable-neon],
		[use the optimisation flag to enable NEON instructions (default: no)])],
	[], [enable_neon="no"])

if test "x$enable_neon" != "xno"; then
	AC_BES_COMPILER_FLAG(["-mfpu=neon"], [mfpu],
	[
		AC_MSG_ERROR(["NEON optimisation flag was not accepted."])
	])

	AC_BES_COMPILER_FLAG(["-Wa,-march=armv7a"],		[march])
	AC_BES_COMPILER_FLAG(["-fprefetch-loop-arrays"],	[prefetch])
	BESFLGS="$BESFLGS -I./OGLES -fsigned-char"
fi

#AC_BES_COMPILER_FLAG([-fno-exceptions],	      [no_exceptions])
#AC_BES_COMPILER_FLAG([-fno-rtti],             [no_rtti])
AC_BES_COMPILER_FLAG([-pedantic],             [pedantic])
AC_BES_COMPILER_FLAG([-Wall],                 [Wall])
AC_BES_COMPILER_FLAG([-W],                    [W])
AC_BES_COMPILER_FLAG([-Wno-unused-parameter], [Wno_unused_parameter])

# Enable GZIP support through zlib.

AC_CACHE_VAL([bes_cv_zlib],
[
	AC_CHECK_HEADER([zlib.h],
		[AC_CHECK_LIB([z], [gzread], [bes_cv_zlib="yes"], [bes_cv_zlib="no"])],
		[bes_cv_zlib="no"])
])

AC_ARG_ENABLE([gzip],
	[AS_HELP_STRING([--enable-gzip],
		[enable GZIP support through zlib (default: yes)])],
	[], [enable_gzip="yes"])

if test "x$enable_gzip" = "xyes"; then
	if test "x$bes_cv_zlib" = "xyes"; then
		BESDEFS="$BESDEFS -DZLIB"
		BESLIBS="$BESLIBS -lz"
	else
		AC_MSG_WARN([zlib not found. Build without GZIP support.])
		enable_gzip="no"
	fi
fi

# Enable screenshot support through libpng.
AC_CACHE_VAL([bes_cv_libpng],
[
	AC_CHECK_HEADER([png.h],
		[AC_CHECK_LIB([png], [png_init_io], [bes_cv_libpng="yes"], [bes_cv_libpng="no"])],
		[bes_cv_libpng="no"])
])

AC_ARG_ENABLE([screenshot],
	[AS_HELP_STRING([--enable-screenshot],
		[enable screenshot support through libpng (default: yes)])],
	[], [enable_screenshot="yes"])

if test "x$enable_screenshot" = "xyes"; then
	if test "x$bes_cv_libpng" = "xyes"; then
		BESDEFS="$BESDEFS -DHAVE_LIBPNG"
		BESLIBS="$BESLIBS -lpng"
	else
		AC_MSG_WARN([libpng not found. Build without screenshot support.])
		enable_screenshot="no"
	fi
fi

# Check for functions

AC_CHECK_FUNC([mkstemp],
[
	BESDEFS="$BESDEFS -DHAVE_MKSTEMP"
])

AC_CACHE_VAL([bes_cv_sdl],
[
	AC_CHECK_HEADER([SDL/SDL.h],
		[AC_CHECK_LIB([SDL], [SDL_Init], [bes_cv_sdl="yes"], [bes_cv_sdl="no"])],
		[bes_cv_sdl="no"])
])

if test "x$bes_cv_sdl" = "xyes"; then
  	BESFLGS="$BESFLGS `sdl-config --cflags`"
	BESLIBS="$BESLIBS `sdl-config --libs` -lX11 -lpthread"
else
	AC_MSG_ERROR([SDL is required.])
fi

# Check for various needed library headers
AC_CHECK_HEADER([SDL/SDL_ttf.h],[],
	AC_MSG_ERROR([SDL_ttf headers not found]))
AC_CHECK_HEADER([SDL/SDL_image.h],[],
	AC_MSG_ERROR([SDL_image headers not found]))
AC_CHECK_HEADER([SDL/SDL_mixer.h],[],
	AC_MSG_ERROR([SDL_mixer headers not found]))

# Check for various needed libraries to link against
AC_CHECK_LIB(SDL_ttf, TTF_OpenFont,
	AC_SUBST(SDL_TTF_LIBS, -lSDL_ttf),
	AC_MSG_ERROR([SDL_ttf library not found]))
BESLIBS="$BESLIBS -lSDL_ttf"

AC_CHECK_LIB(SDL_image, IMG_Load,
	AC_SUBST(SDL_IMAGE_LIBS, -lSDL_image),
	AC_MSG_ERROR([SDL_image library not found]))
BESLIBS="$BESLIBS -lSDL_image"

AC_CHECK_LIB(SDL_mixer, Mix_OpenAudio,
	AC_SUBST(SDL_MIXER_LIBS, -lSDL_mixer),
	AC_MSG_ERROR([SDL_mixer library not found]))
BESLIBS="$BESLIBS -lSDL_mixer"

AC_CHECK_LIB(EGL, eglGetDisplay,
	AC_SUBST(EGL_LIBS, -lEGL),
	AC_MSG_ERROR([EGL library not found]))
BESLIBS="$BESLIBS -lEGL"

AC_CHECK_LIB(GLESv1_CM, glEnable,
	AC_SUBST(EGL_LIBS, -lGLESv1_CM),
	AC_MSG_ERROR([GLES library not found]))
BESLIBS="$BESLIBS -lGLESv1_CM"

# Check for headers

bes_have_stdint_h="";

AC_CHECK_HEADER([strings.h],
[
	BESDEFS="$BESDEFS -DHAVE_STRINGS_H"
])

AC_CHECK_HEADER([sys/ioctl.h],
[
	BESDEFS="$BESDEFS -DHAVE_SYS_IOCTL_H"
])

AC_CHECK_HEADER([stdint.h],
[
	BESDEFS="$BESDEFS -DHAVE_STDINT_H"
	bes_have_stdint_h="-DHAVE_STDINT_H"
])

AC_CHECK_HEADERS([unistd.h sys/socket.h])

# Check whether the size of pointer is int.

if test "x$bes_have_stdint_h" = "x"; then
	AC_MSG_CHECKING([whether the size of pointer is int])
	
	AC_TRY_RUN(
	[
		int main (void)
		{
			return (!(sizeof(void *) == sizeof(int)));
		}
	],
	[bes_ptr_is_int="yes"], [bes_ptr_is_int="no"])
	
	if test "x$bes_ptr_is_int" = "xyes"; then
		AC_MSG_RESULT(yes)
	else
		AC_MSG_RESULT(no)
		BESDEFS="$BESDEFS -DPTR_NOT_INT"
	fi
fi

# Output.

BESFLGS="$CXXFLAGS $CPPFLAGS $LDFLAGS $BESFLGS"
BESLIBS="$LIBS $BESLIBS"

BESFLGS="`echo \"$BESFLGS\" | sed -e 's/  */ /g'`"
BESDEFS="`echo \"$BESDEFS\" | sed -e 's/  */ /g'`"
BESLIBS="`echo \"$BESLIBS\" | sed -e 's/  */ /g'`"
BESFLGS="`echo \"$BESFLGS\" | sed -e 's/^  *//'`"
BESDEFS="`echo \"$BESDEFS\" | sed -e 's/^  *//'`"
BESLIBS="`echo \"$BESLIBS\" | sed -e 's/^  *//'`"

AC_SUBST(BESFLGS)
AC_SUBST(BESDEFS)
AC_SUBST(BESLIBS)

rm config.info 2>/dev/null

cat >config.info <<EOF

build information:
cc...............,,,. $CC
c++.................. $CXX
options.............. $BESFLGS
defines.............. $BESDEFS
libs................. $BESLIBS

features:
screenshot support... $enable_screenshot
GZIP support......... $enable_gzip

EOF

cat config.info

AC_OUTPUT(Makefile)
