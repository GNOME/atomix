project('atomix', ['c'],
  version: '3.30.0.1',
  meson_version: '>= 0.41',
  license: 'GPLv2+',
)

gnome = import('gnome')
i18n = import('i18n')

localedir = join_paths(get_option('prefix'), get_option('localedir'))
datadir = join_paths (get_option ('prefix'), get_option ('datadir'))
pkgdatadir = join_paths (datadir, 'atomix')

podir = join_paths(meson.source_root(), 'po')
itsdir = join_paths(meson.source_root(), 'data')
# Dependencies

gtk_dep = dependency('gtk+-3.0', version: '>= 3.10')
gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0', version: '>= 2.0.5')
glib_dep = dependency('glib-2.0', version: '>= 2.36.0')
libgnome_games_support_dep = dependency('libgnome-games-support-1')

cc = meson.get_compiler('c')

conf = configuration_data()

conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf.set_quoted('PACKAGE_URL', 'https://wiki.gnome.org/Apps/Atomix')
conf.set_quoted('VERSION', meson.project_version())

config_h_inc = include_directories('.')

# Subdirs
subdir('data')
subdir('src')
subdir('po')

config_file = configure_file(output: 'config.h', configuration: conf)
# Extra scripts
meson.add_install_script('meson_post_install.py', datadir)
