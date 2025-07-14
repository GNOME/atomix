# Icons
install_subdir('icons',
  install_dir: datadir,
)

# UI files
install_data('atomix-logo.png',
  install_dir: pkgdatadir,
)

install_subdir('ui',
  install_dir: pkgdatadir,
)

# Themes
install_subdir('themes',
  install_dir: pkgdatadir,
)


# D-Bus service file
service_conf = configuration_data()
service_conf.set('app_id', app_id)
service_conf.set('bindir', bindir)

configure_file(
          input: 'org.gnome.Atomix.service.in',
         output: '@0@.service'.format(app_id),
        install: true,
    install_dir: join_paths(datadir, 'dbus-1', 'services'),
  configuration: service_conf
)


# Desktop file
desktop_file_in_config = configuration_data()
desktop_file_in_config.set('app_id', app_id)
desktop_file_in = configure_file(
          input: 'org.gnome.Atomix.desktop.in',
         output: '@0@.desktop.in'.format(app_id),
  configuration: desktop_file_in_config,
)

desktop_file = i18n.merge_file(
        input: desktop_file_in,
       output: '@0@.desktop'.format(app_id),
         type: 'desktop',
       po_dir: podir,
      install: true,
  install_dir: join_paths(get_option('datadir'), 'applications')
)


# Metainfo
appconf = configuration_data()
appconf.set('app_id', app_id)
appstream_file_in = configure_file(
          input: 'org.gnome.Atomix.metainfo.xml.in',
         output: '@0@.metainfo.xml.in'.format(app_id),
  configuration: appconf,
)

appstream_file = i18n.merge_file(
          input: appstream_file_in,
         output: '@0@.metainfo.xml'.format(app_id),
         po_dir: podir,
        install: true,
    install_dir: join_paths(get_option('datadir'), 'metainfo')
)

# Validate Metainfo
appstreamcli = find_program('appstreamcli', required: false)
if (appstreamcli.found())
  test('Validating metainfo',
    appstreamcli,
    args: ['validate', '--no-net', appstream_file],
    workdir: meson.current_build_dir()
  )
endif


subdir('level')
