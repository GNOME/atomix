AC_PREREQ([2.63])

AC_INIT([atomix],[3.22.0],[https://gitlab.gnome.org/GNOME/atomix/issues/new?issue])

AX_IS_RELEASE([minor-version])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.10 foreign no-dist-gzip dist-xz])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

# Check for programs
AC_PROG_CC
AC_PROG_RANLIB

AM_PROG_AR

AX_REQUIRE_DEFINED([APPSTREAM_XML])
APPSTREAM_XML

AX_COMPILER_FLAGS

dnl ================= Requirements =======================
LIBGTK_REQUIRED=3.10.0
GLIB_REQUIRED=2.36.0
GDK_PIXBUF_REQUIRED=2.0.5

dnl ******************************
dnl pkg-config checks
dnl ******************************

ATOMIX_MODULES="gtk+-3.0 >= $LIBGTK_REQUIRED gdk-pixbuf-2.0 >= $GDK_PIXBUF_REQUIRED glib-2.0 >= $GLIB_REQUIRED"
PKG_CHECK_MODULES(ATOMIX, $ATOMIX_MODULES)
AC_SUBST(ATOMIX_CFLAGS)
AC_SUBST(ATOMIX_LIBS)

dnl *******************************
dnl Internationalization
dnl *******************************

IT_PROG_INTLTOOL([0.40.0])

GETTEXT_PACKAGE=atomix
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],["$GETTEXT_PACKAGE"],[Gettext domain])
AC_SUBST(GETTEXT_PACKAGE)

AC_CONFIG_FILES([
Makefile
po/Makefile.in
src/Makefile
data/Makefile
data/icons/Makefile
data/icons/hicolor/Makefile
data/themes/Makefile
data/themes/default/Makefile
data/level/Makefile
data/ui/Makefile
])

AC_OUTPUT
