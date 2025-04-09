project('atomix', ['c'],
  version: '48.alpha',
  meson_version: '>= 0.57',
  license: 'GPLv2+',
)

gnome = import('gnome')
i18n = import('i18n')

if get_option('development')
  app_id = 'org.gnome.Atomix.Devel'
  name_prefix = '(Nightly) '
  vcs_tag = run_command('git', 'rev-parse', '--short', 'HEAD').stdout().strip()
  version_suffix = '-@0@'.format (vcs_tag)
else
  app_id = 'org.gnome.Atomix'
  name_prefix = ''
  version_suffix = ''
endif

localedir = join_paths(get_option('prefix'), get_option('localedir'))
datadir = join_paths (get_option ('prefix'), get_option ('datadir'))
pkgdatadir = join_paths (datadir, 'atomix')
bindir = join_paths (get_option ('prefix'), get_option ('bindir'))

podir = join_paths(meson.project_source_root(), 'po')
itsdir = join_paths(meson.project_source_root(), 'data')
# Dependencies

gtk_dep = dependency('gtk+-3.0', version: '>= 3.24')
gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0', version: '>= 2.0.5')
glib_dep = dependency('glib-2.0', version: '>= 2.36.0')
libgnome_games_support_dep = dependency('libgnome-games-support-1')

cc = meson.get_compiler('c')
libm_dep = cc.find_library('m', required : false)

conf = configuration_data()

conf.set_quoted('APP_ID', app_id)
conf.set_quoted('PACKAGE_URL', 'https://wiki.gnome.org/Apps/Atomix')
conf.set_quoted('NAME_PREFIX', name_prefix)
conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf.set_quoted('VERSION', meson.project_version() + version_suffix)
conf.set10('DEVELOPMENT_BUILD', get_option('development'))


config_h_inc = include_directories('.')

# Subdirs
subdir('data')
subdir('src')
subdir('po')

config_file = configure_file(output: 'config.h', configuration: conf)
# Extra scripts
meson.add_install_script('meson_post_install.py', datadir)
