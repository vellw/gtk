node_editor_sources = [
  'gtkrendererpaintable.c',
  'main.c',
  'node-editor-application.c',
  'node-editor-window.c',
]

node_editor_resources = gnome.compile_resources('node_editor_resources',
                                                'node-editor.gresource.xml',
                                                source_dir: '.')

executable('gtk4-node-editor',
           node_editor_sources, node_editor_resources,
           dependencies: libgtk_dep,
           include_directories: confinc,
           c_args: [
             '-DNODE_EDITOR_SOURCE_DIR="@0@/../../testsuite/gsk/compare/"'.format(meson.current_source_dir())
           ] + common_cflags,
           gui_app: true,
           link_args: extra_demo_ldflags,
           install: false)

# icons
icontheme_dir = join_paths(gtk_datadir, 'icons/hicolor')

foreach size: ['scalable', 'symbolic']
  install_subdir('data/' + size,
    install_dir: icontheme_dir
  )
endforeach
