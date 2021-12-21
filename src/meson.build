raider_sources = [
  'main.c',
  'raider-window.c',
  'raider-window.h',
  'raider.c',
  'raider.h',
  'raider-preferences.c',
  'raider-preferences.h',
  'raider-file-row.c',
  'raider-file-row.h'
]

raider_deps = [
  dependency('gio-2.0', version: '>= 2.50'),
  dependency('gtk+-3.0', version: '>= 3.22'),
]

gnome = import('gnome')

subdir('theme')

raider_sources += gnome.compile_resources('raider-resources',
  'raider.gresource.xml',
  c_name: 'raider'
)

executable('raider', raider_sources,
  dependencies: raider_deps,
  install: true,
)
