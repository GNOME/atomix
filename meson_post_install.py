#!/usr/bin/env python3

import os
import subprocess
import sys

if not os.environ.get('DESTDIR'):
  icon_dir = os.path.join(sys.argv[1], 'icons', 'hicolor')
#  schema_dir = os.path.join(sys.argv[1], 'glib-2.0', 'schemas')
  print('Update icon cache...')
  subprocess.call(['gtk-update-icon-cache', '-f', '-t', icon_dir])

#  print('Compiling gsettings schemas...')
#  subprocess.call(['glib-compile-schemas', schema_dir])

