levelsdir = join_paths(pkgdatadir, 'level')

# Level files
levels = [
          'aceticacid.atomix.xml',
          'acetone.atomix.xml',
          'butanol.atomix.xml',
          'cyclobutane.atomix.xml',
          'dimethylether.atomix.xml',
          'ethane.atomix.xml',
          'ethanal.atomix.xml',
          'ethanol.atomix.xml',
          'ethylene.atomix.xml',
          'glycerin.atomix.xml',
          'lactic-acid.atomix.xml',
          'methanal.atomix.xml',
          'methane.atomix.xml',
          'methanol.atomix.xml',
          'propanal.atomix.xml',
          'propylene.atomix.xml',
          'pyran.atomix.xml',
          'transbutylen.atomix.xml',
          'water.atomix.xml',
         ]

# Translate level files
foreach level : levels
  i18n.merge_file(
    input: level + '.in',
    output: level,
    type: 'xml',
    po_dir: podir,
    install: true,
    install_dir: levelsdir,
    data_dirs: itsdir,
  )
endforeach

# Level sequence file
install_data('sequence',
  install_dir: levelsdir,
)
