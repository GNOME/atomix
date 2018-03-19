# Icons
install_subdir('icons',
  install_dir: datadir,
  exclude_files: [ 'Makefile', 'Makefile.in', 'Makefile.am', 
                   'hicolor/Makefile', 'hicolor/Makefile.in', 'hicolor/Makefile.am', '.gitignore' ],
)

# UI files
install_data('atomix-logo.png',
  install_dir: pkgdatadir,
)

install_subdir('ui',
  install_dir: pkgdatadir,
  exclude_files: [ 'Makefile', 'Makefile.in', 'Makefile.am', '.gitignore' ],
)

# Themes
install_subdir('themes',
  install_dir: pkgdatadir,
  exclude_files: [ 'Makefile', 'Makefile.in', 'Makefile.am', 
                   'default/Makefile', 'default/Makefile.in', 'default/Makefile.am'],
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
