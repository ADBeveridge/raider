raider_sources = [
  'main.c',
  'raider-window.c',
  'raider-application.c',
  'raider-progress-icon.c',
  'raider-file-row.c',
  'raider-preferences.c',
  'raider-shred-backend.c',
  'raider-progress-info-popover.c'
]

# Generate UX files from blueprint files.
blueprints = custom_target('blueprints',
  input: files(
    'raider-window.blp',
    'raider-file-row.blp',
    'raider-preferences.blp',
    'raider-progress-info-popover.blp',
    'raider-shortcuts.blp',
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
