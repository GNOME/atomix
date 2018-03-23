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

# Desktop file
desktop = 'atomix.desktop'

# Desktop file
i18n.merge_file(
  input: desktop + '.in',
  output: desktop,
  type: 'desktop',
  po_dir: podir,
  install: true,
  install_dir: join_paths(datadir, 'applications'),
)

# Appdata
appdata = 'atomix.appdata.xml'

i18n.merge_file(
  input: appdata + '.in',
  output: appdata,
  po_dir: podir,
  install: true,
  install_dir: join_paths(datadir, 'metainfo')
)

subdir('level')
