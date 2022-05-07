app_sources = [
    'atomix-application.h',
    'main.c', 'main.h',
    'board-gtk.c', 'board-gtk.h',
    'undo.c', 'undo.h',
    'goal.c', 'goal.h',
    'goal-view.c', 'goal-view.h',
    'clock.c', 'clock.h',
]

app_c_flags = [
  '-DPKGDATADIR="@0@"'.format(pkgdatadir),
  '-DDATADIR="@0@"'.format(datadir),
  '-DGNOMELOCALEDIR="@0@"'.format(localedir),
  '-DGETTEXT_PACKAGE="@0@"'.format(meson.project_name())
]

library_sources = [
    'tile.c','tile.h',
    'xml-util.c','xml-util.h',
    'theme.c','theme.h','theme-private.h',
    'theme-manager.c','theme-manager.h',
    'level.c','level.h','level-private.h',
    'level-manager.c','level-manager.h',
    'canvas_helper.c','canvas_helper.h',
    'playfield.c','playfield.h',
]

app_dependencies = [
    gtk_dep,
    glib_dep,
    gdk_pixbuf_dep,
    libgnome_games_support_dep,
    libm_dep,
]

atomix_lib = static_library('atomix', library_sources,
    install: false,
    dependencies: app_dependencies,
    c_args: app_c_flags,
)

executable('atomix', app_sources,
    dependencies: app_dependencies,
    link_with:atomix_lib,
    include_directories: config_h_inc,
    c_args: app_c_flags,
    install: true,
)

playfield_test_sources = [
    'playfield.h', 'playfield.c',
    'tile.c', 'tile.h',
    'theme.c', 'theme.h',
    'xml-util.c', 'xml-util.h',
    'pf-test.c'
]

executable('pf-test', playfield_test_sources,
    dependencies: app_dependencies,
    link_with:atomix_lib,
    include_directories: config_h_inc,
    c_args: app_c_flags,
    install: false,
)
