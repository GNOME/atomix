AC_PREREQ([2.63])

AC_INIT([atomix],[2.14.0],[http://bugzilla.gnome.org/enter_bug.cgi?product=atomix])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/board.c])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.10 foreign no-dist-gzip dist-bzip2])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

# Check for programs
AC_PROG_CC
AC_PROG_RANLIB

GNOME_COMPILE_WARNINGS([maximum])
GNOME_MAINTAINER_MODE_DEFINES

dnl ================= Requirements =======================
LIBGTK_REQUIRED=2.24.0
LIBXML_REQUIRED=2.4.23
GLIB_REQUIRED=2.36.0
GDK_PIXBUF_REQUIRED=2.0.5
LIBGNOMECANVAS_REQUIRED=2.0.1

dnl ******************************
dnl pkg-config checks
dnl ******************************

ATOMIX_MODULES="gtk+-2.0 >= $LIBGTK_REQUIRED libxml-2.0 >= $LIBXML_REQUIRED gdk-pixbuf-2.0 >= $GDK_PIXBUF_REQUIRED libgnomecanvas-2.0 >= $LIBGNOMECANVAS_REQUIRED glib-2.0 >= $GLIB_REQUIRED"
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
themes/Makefile
themes/default/Makefile
level/Makefile])

AC_OUTPUT
