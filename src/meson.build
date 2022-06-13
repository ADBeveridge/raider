raider_sources = [
  'main.c',
  'raider-window.c',
  'raider-application.c',
  'raider-file-row.c',
  'raider-preferences.c',
  'raider-progress-icon.c'
]

raider_deps = [
  dependency('gtk4'),
  dependency('libadwaita-1', version: '>= 1.0'),
]

gnome = import('gnome')

# Generate UI files from blueprint files.
blueprints = custom_target('blueprints',
  input: files(
    'raider-window.blp',
    'raider-file-row.blp',
    'raider-preferences.blp',
  ),
  output: '.',
  command: [find_program('blueprint-compiler'), 'batch-compile', '@OUTPUT@', '@CURRENT_SOURCE_DIR@', '@INPUT@'],
)

raider_sources += gnome.compile_resources('raider-resources',
  'raider.gresource.xml',
  dependencies: blueprints,
  c_name: 'raider'
)

executable('raider', raider_sources,
  dependencies: raider_deps,
  install: true,
)
