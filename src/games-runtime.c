/*
 * Copyright © 2007 Andreas Røsdal <andreasr@gnome.org>
 * Copyright © 2007, 2008 Christian Persch
 * Copyright © 2009 Tor Lillqvist
 *
 * This game is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This runtime is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this runtime; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 * USA
 */

#include <config.h>

#include <locale.h>

#ifdef G_OS_WIN32
#include <io.h>
#include <conio.h>
#define _WIN32_WINNT 0x0500
#include <windows.h>
#endif /* G_OS_WIN32 */

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "games-debug.h"
#include "games-profile.h"
#include "games-show.h"

#include "games-runtime.h"

#ifdef HAVE_HILDON
static osso_context_t *osso_context;
#endif

#if defined(G_OS_WIN32) && !defined(ENABLE_BINRELOC)
#error binreloc must be enabled on win32
#endif

#if defined(ENABLE_BINRELOC) && !defined(G_OS_WIN32)

/*
 * BinReloc - a library for creating relocatable executables
 * Written by: Hongli Lai <h.lai@chello.nl>
 * http://autopackage.org/
 *
 * This source code is public domain. You can relicense this code
 * under whatever license you want.
 *
 * See http://autopackage.org/docs/binreloc/ for
 * more information and how to use this.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/** These error codes can be returned by br_init(), br_init_lib(), gbr_init() or gbr_init_lib(). */
typedef enum {
	/** Cannot allocate memory. */
	GBR_INIT_ERROR_NOMEM,
	/** Unable to open /proc/self/maps; see errno for details. */
	GBR_INIT_ERROR_OPEN_MAPS,
	/** Unable to read from /proc/self/maps; see errno for details. */
	GBR_INIT_ERROR_READ_MAPS,
	/** The file format of /proc/self/maps is invalid; kernel bug? */
	GBR_INIT_ERROR_INVALID_MAPS,
	/** BinReloc is disabled (the ENABLE_BINRELOC macro is not defined). */
	GBR_INIT_ERROR_DISABLED
} GbrInitError;

/** @internal
 * Find the canonical filename of the executable. Returns the filename
 * (which must be freed) or NULL on error. If the parameter 'error' is
 * not NULL, the error code will be stored there, if an error occured.
 */
static char *
_br_find_exe (GbrInitError *error)
{
	char *path, *path2, *line, *result;
	size_t buf_size;
	ssize_t size;
	struct stat stat_buf;
	FILE *f;

	/* Read from /proc/self/exe (symlink) */
	if (sizeof (path) > SSIZE_MAX)
		buf_size = SSIZE_MAX - 1;
	else
		buf_size = PATH_MAX - 1;
	path = (char *) g_try_malloc (buf_size);
	if (path == NULL) {
		/* Cannot allocate memory. */
		if (error)
			*error = GBR_INIT_ERROR_NOMEM;
		return NULL;
	}
	path2 = (char *) g_try_malloc (buf_size);
	if (path2 == NULL) {
		/* Cannot allocate memory. */
		if (error)
			*error = GBR_INIT_ERROR_NOMEM;
		g_free (path);
		return NULL;
	}

	strncpy (path2, "/proc/self/exe", buf_size - 1);

	while (1) {
		int i;

		size = readlink (path2, path, buf_size - 1);
		if (size == -1) {
			/* Error. */
			g_free (path2);
			break;
		}

		/* readlink() success. */
		path[size] = '\0';

		/* Check whether the symlink's target is also a symlink.
		 * We want to get the final target. */
		i = stat (path, &stat_buf);
		if (i == -1) {
			/* Error. */
			g_free (path2);
			break;
		}

		/* stat() success. */
		if (!S_ISLNK (stat_buf.st_mode)) {
			/* path is not a symlink. Done. */
			g_free (path2);
			return path;
		}

		/* path is a symlink. Continue loop and resolve this. */
		strncpy (path, path2, buf_size - 1);
	}


	/* readlink() or stat() failed; this can happen when the program is
	 * running in Valgrind 2.2. Read from /proc/self/maps as fallback. */

	buf_size = PATH_MAX + 128;
	line = (char *) g_try_realloc (path, buf_size);
	if (line == NULL) {
		/* Cannot allocate memory. */
		g_free (path);
		if (error)
			*error = GBR_INIT_ERROR_NOMEM;
		return NULL;
	}

	f = fopen ("/proc/self/maps", "r");
	if (f == NULL) {
		g_free (line);
		if (error)
			*error = GBR_INIT_ERROR_OPEN_MAPS;
		return NULL;
	}

	/* The first entry should be the executable name. */
	result = fgets (line, (int) buf_size, f);
	if (result == NULL) {
		fclose (f);
		g_free (line);
		if (error)
			*error = GBR_INIT_ERROR_READ_MAPS;
		return NULL;
	}

	/* Get rid of newline character. */
	buf_size = strlen (line);
	if (buf_size <= 0) {
		/* Huh? An empty string? */
		fclose (f);
		g_free (line);
		if (error)
			*error = GBR_INIT_ERROR_INVALID_MAPS;
		return NULL;
	}
	if (line[buf_size - 1] == 10)
		line[buf_size - 1] = 0;

	/* Extract the filename; it is always an absolute path. */
	path = strchr (line, '/');

	/* Sanity check. */
	if (strstr (line, " r-xp ") == NULL || path == NULL) {
		fclose (f);
		g_free (line);
		if (error)
			*error = GBR_INIT_ERROR_INVALID_MAPS;
		return NULL;
	}

	path = g_strdup (path);
	g_free (line);
	fclose (f);
	return path;
}

#endif /* ENABLE_BINRELOC && !G_OS_WIN32 */

static char *app_name;
static int gpl_version;
static char *cached_directories[GAMES_RUNTIME_LAST_DIRECTORY];

typedef struct {
  GamesRuntimeDirectory base_dir;
  const char *name;
} DerivedDirectory;

static const DerivedDirectory derived_directories[] = {
  /* Keep this in the same order as in the GamesRuntimeDirectory enum! */
#ifdef ENABLE_BINRELOC
  { GAMES_RUNTIME_MODULE_DIRECTORY,   "share"              }, /* GAMES_RUNTIME_DATA_DIRECTORY              */
  { GAMES_RUNTIME_DATA_DIRECTORY,     "gnome-games-common" }, /* GAMES_RUNTIME_COMMON_DATA_DIRECTORY       */
  { GAMES_RUNTIME_DATA_DIRECTORY,     "gnome-games"        }, /* GAMES_RUNTIME_PKG_DATA_DIRECTORY          */
  { GAMES_RUNTIME_DATA_DIRECTORY,     "scores"             }, /* GAMES_RUNTIME_SCORES_DIRECTORY            */
#endif /* ENABLE_BINRELOC */
  { GAMES_RUNTIME_DATA_DIRECTORY,         "locale"         }, /* GAMES_RUNTIME_LOCALE_DIRECTORY            */
  { GAMES_RUNTIME_COMMON_DATA_DIRECTORY,  "pixmaps"        }, /* GAMES_RUNTIME_COMMON_PIXMAP_DIRECTORY     */
  { GAMES_RUNTIME_COMMON_DATA_DIRECTORY,  "card-themes"    }, /* GAMES_RUNTIME_PRERENDERED_CARDS_DIRECTORY */
  { GAMES_RUNTIME_COMMON_DATA_DIRECTORY,  "cards"          }, /* GAMES_RUNTIME_SCALABLE_CARDS_DIRECTORY    */
  { GAMES_RUNTIME_PKG_DATA_DIRECTORY,     "icons"          }, /* GAMES_RUNTIME_ICON_THEME_DIRECTORY        */
  { GAMES_RUNTIME_PKG_DATA_DIRECTORY,     "pixmaps"        }, /* GAMES_RUNTIME_PIXMAP_DIRECTORY            */
  { GAMES_RUNTIME_PKG_DATA_DIRECTORY,     "sounds"         }, /* GAMES_RUNTIME_SOUNDS_DIRECTORY            */
  { GAMES_RUNTIME_PKG_DATA_DIRECTORY,     NULL             }, /* GAMES_RUNTIME_GAME_DATA_DIRECTORY         */
  { GAMES_RUNTIME_GAME_DATA_DIRECTORY,    "games"          }, /* GAMES_RUNTIME_GAME_GAMES_DIRECTORY        */
  { GAMES_RUNTIME_GAME_DATA_DIRECTORY,    "pixmaps"        }, /* GAMES_RUNTIME_GAME_PIXMAP_DIRECTORY       */
  { GAMES_RUNTIME_GAME_DATA_DIRECTORY,    "themes"         }, /* GAMES_RUNTIME_GAME_THEME_DIRECTORY        */
  { GAMES_RUNTIME_GAME_DATA_DIRECTORY,    "help"           }, /* GAMES_RUNTIME_GAME_HELP_DIRECTORY         */
};

typedef int _assertion[G_N_ELEMENTS (derived_directories) + GAMES_RUNTIME_FIRST_DERIVED_DIRECTORY == GAMES_RUNTIME_LAST_DIRECTORY ? 1 : -1];

#if !GTK_CHECK_VERSION (2, 17, 0)
/* Since version 2.17.0, gtk has default about dialogue hook functions
 * using gtk_show_uri(). For earlier versions, we need to implement
 * our own hooks.
 */

static void
about_url_hook (GtkAboutDialog *about,
                const char *uri,
                gpointer user_data)
{
  GdkScreen *screen;
  GError *error = NULL;

  screen = gtk_widget_get_screen (GTK_WIDGET (about));

  if (!games_show_uri (screen, uri, gtk_get_current_event_time (), &error)) {
    games_show_error (GTK_WIDGET (about),
                      error,
                      "%s", _("Could not show link"));
    g_error_free (error);
  }
}

static void
about_email_hook (GtkAboutDialog *about,
                  const char *email_address,
                  gpointer user_data)
{
  char *uri;

#if GLIB_CHECK_VERSION (2, 16, 0)
  char *escaped_email_address;

  escaped_email_address = g_uri_escape_string (email_address, NULL, FALSE);
  uri = g_strdup_printf ("mailto:%s", escaped_email_address);
  g_free (escaped_email_address);
#else
  /* Not really correct, but the best we can do */
  uri = g_strdup_printf ("mailto:%s", email_address);
#endif

  about_url_hook (about, uri, user_data);
  g_free (uri);
}

#endif /* GTK < 2.17.0 */

/* public API */

/**
 * games_runtime_init:
 *
 * Initialises the runtime file localisator. This also calls setlocale,
 * and initialises gettext support and gnome-games debug support.
 *
 * NOTE: This must be called before using ANY other glib/gtk/etc function!
 * 
 * Returns: %TRUE iff initialisation succeeded
 */
gboolean
games_runtime_init (const char *name)
{
  gboolean retval;

  setlocale (LC_ALL, "");

#ifdef G_OS_WIN32
  /* On Windows, when called from a console, get console output. This works
   * only with Windows XP or higher; Windows 2000 will not have console
   * output but it will just work fine.
   */
  if (fileno (stdout) != -1 &&
      _get_osfhandle (fileno (stdout)) != -1) {
    /* stdout is fine, presumably redirected to a file or pipe.
     * Make sure stdout goes somewhere, too.
     */
    if (_get_osfhandle (fileno (stderr)) == -1) {
      dup2 (fileno (stdout), fileno (stderr));
    }
  } else {
    typedef BOOL (* WINAPI AttachConsole_t) (DWORD);

    AttachConsole_t p_AttachConsole =
      (AttachConsole_t) GetProcAddress (GetModuleHandle ("kernel32.dll"), "AttachConsole");

    if (p_AttachConsole != NULL && p_AttachConsole (ATTACH_PARENT_PROCESS)) {
      freopen ("CONOUT$", "w", stdout);
      dup2 (fileno (stdout), 1);
      freopen ("CONOUT$", "w", stderr);
      dup2 (fileno (stderr), 2);
    }
  }
#endif /* G_OS_WIN32 */

#if defined(HAVE_GNOME) || defined(HAVE_RSVG_GNOMEVFS) || defined(HAVE_CANBERRA_GTK)
  /* If we're going to use gconf, gnome-vfs, or canberra, we need to
   * init threads; and this has to be done before calling any other glib functions.
   */
#if defined(LIBGAMES_SUPPORT_GI)
  /* Seed has already called g_thread_init() */
  g_assert (g_thread_get_initialized());
#else
  g_thread_init (NULL);
#endif
#endif
  /* May call any glib function after this point */

  _games_profile_start ("games_runtime_init");

  _games_debug_init ();

  app_name = g_strdup (name);

  bindtextdomain (GETTEXT_PACKAGE, games_runtime_get_directory (GAMES_RUNTIME_LOCALE_DIRECTORY));
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain(GETTEXT_PACKAGE);

#ifdef ENABLE_BINRELOC
{
  const char *path;

  /* Now check that we can get the module installation directory */
  path = games_runtime_get_directory (GAMES_RUNTIME_MODULE_DIRECTORY);

  _games_debug_print (GAMES_DEBUG_RUNTIME,
                      "Relocation path: %s\n", path ? path : "(null)");

  retval = path != NULL;
}
#else /* !ENABLE_BINRELOC */
  retval = TRUE;
#endif /* ENABLE_BINRELOC */

  if (strcmp (app_name, "aisleriot") == 0) {
    gpl_version = 3;
  } else {
    gpl_version = 2;
  }

#if !GTK_CHECK_VERSION (2, 17, 0)
  gtk_about_dialog_set_url_hook (about_url_hook, NULL, NULL);
  gtk_about_dialog_set_email_hook (about_email_hook, NULL, NULL);
#endif

  _games_profile_end ("games_runtime_init");

  return retval;
}

#ifdef HAVE_HILDON

/**
 * games_runtime_init_with_osso:
 *
 * Like games_runtime_init(), but also initialises the osso context.
 *
 * NOTE: This must be called before using ANY other glib/gtk/etc function!
 * 
 * Returns: %TRUE iff initialisation succeeded
 */
gboolean
games_runtime_init_with_osso (const char *name,
                              const char *service_name)
{
  if (!games_runtime_init (name))
    return FALSE;

  osso_context = osso_initialize (service_name, VERSION, FALSE, NULL);
  if (osso_context == NULL) {
    g_printerr ("Failed to initialise osso\n");
    return FALSE;
  }

  return TRUE;
}

/**
 * games_runtime_get_osso_context:
 *
 * Returns the osso context. May only be called after
 * games_runtime_init_with_osso().
 *
 * Returns: a #osso_context_t
 */
osso_context_t*
games_runtime_get_osso_context (void)
{
  g_assert (osso_context != NULL);
  return osso_context;
}

#endif /* HAVE_HILDON */

/**
 * games_runtime_shutdown:
 *
 * Shuts down the runtime file localator.
 */
void
games_runtime_shutdown (void)
{
  guint i;

  for (i = 0; i < GAMES_RUNTIME_LAST_DIRECTORY; ++i) {
    g_free (cached_directories[i]);
    cached_directories[i] = NULL;
  }

  g_free (app_name);
  app_name = NULL;

#ifdef HAVE_HILDON
  if (osso_context != NULL) {
    osso_deinitialize (osso_context);
    osso_context = NULL;
  }
#endif /* HAVE_HILDON */
}

/**
 * games_runtime_get_directory:
 * @runtime: the #GamesProgram instance
 * @directory:
 *
 * Returns: the path to use for @directory
 */
const char *
games_runtime_get_directory (GamesRuntimeDirectory directory)
{

  char *path = NULL;

  g_return_val_if_fail (app_name != NULL, NULL);
  g_return_val_if_fail (directory < GAMES_RUNTIME_LAST_DIRECTORY, NULL);

  if (cached_directories[directory])
    return cached_directories[directory];

  switch ((int) directory) {
#ifdef ENABLE_BINRELOC
    case GAMES_RUNTIME_MODULE_DIRECTORY:
#ifdef G_OS_WIN32
      path = g_win32_get_package_installation_directory_of_module (NULL);
#else
      {
        GbrInitError errv = 0;
        const char *env;

        if ((env = g_getenv ("GAMES_RELOC_ROOT")) != NULL) {
          path = g_strdup (env);
        } else {
          char *exe, *bindir, *prefix;

          exe = _br_find_exe (&errv);
          if (exe == NULL) {
            g_printerr ("Failed to locate the binary relocation prefix (error code %u)\n", errv);
          } else {
            bindir = g_path_get_dirname (exe);
            g_free (exe);
            prefix = g_path_get_dirname (bindir);
            g_free (bindir);

            if (prefix != NULL && strcmp (prefix, ".") != 0) {
              path = prefix;
            } else {
              g_free (prefix);
            }
          }
        }
      }
#endif /* G_OS_WIN32 */
      break;
#else /* !ENABLE_BINRELOC */

    case GAMES_RUNTIME_DATA_DIRECTORY:
      path = g_strdup (DATADIR);
      break;

    case GAMES_RUNTIME_COMMON_DATA_DIRECTORY:
      path = g_build_filename (DATADIR, "gnome-games-common", NULL);
      break;

    case GAMES_RUNTIME_PKG_DATA_DIRECTORY:
      path = g_strdup (PKGDATADIR);
      break;

    case GAMES_RUNTIME_SCORES_DIRECTORY:
      path = g_strdup (SCORESDIR);
      break;
#endif /* ENABLE_BINRELOC */

    default: {
      const DerivedDirectory *base = &derived_directories[directory - GAMES_RUNTIME_FIRST_DERIVED_DIRECTORY];

      path = g_build_filename (games_runtime_get_directory (base->base_dir),
                               base->name ? base->name : app_name,
                               NULL);
    }
  }

  cached_directories[directory] = path;
  return path;
}

/**
 * games_runtime_get_file:
 * @runtime: the #GamesProgram instance
 * @directory:
 * @name:
 *
 * Returns: a newly allocated string containing the path to the file with name @name
 *   in the directory specicifed by @directory
 */
char *
games_runtime_get_file (GamesRuntimeDirectory directory,
                        const char *name)
{
  const char *dir;

  g_return_val_if_fail (app_name != NULL, NULL);

  dir = games_runtime_get_directory (directory);
  if (!dir)
    return NULL;

  return g_build_filename (dir, name, NULL);
}

/**
 * games_runtime_get_gpl_version:
 *
 * Returns: the minimum GPL version that the executable is licensed under
 */
int
games_runtime_get_gpl_version (void)
{
  return gpl_version;
}
