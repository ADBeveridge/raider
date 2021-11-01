raider_sources = [
  'main.c',
  'raider-window.c',
  'raider.c',
  'raider-add-file.c',
  'raider.h',
  'raider-window.h',
  'raider-add-file.h',
  'raider-shred-file.h',
  'raider-shred-file.c',
  'raider-preferences.c',
  'raider-preferences.h',
  'raider-shred-parser.c',
  'raider-shred-parser.h',
  'raider-pass-data.h'
]

raider_deps = [
  dependency('gio-2.0', version: '>= 2.50'),
  dependency('gtk+-3.0', version: '>= 3.22'),
]

gnome = import('gnome')

raider_sources += gnome.compile_resources('raider-resources',
  'raider.gresource.xml',
  c_name: 'raider'
)

executable('raider', raider_sources,
  dependencies: raider_deps,
  install: true,
)
