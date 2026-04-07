corrupt_sources = [
  'src/corrupt.c',
  'src/bucket.c',
  'src/utility.c'
]

corrupt_public_inc = include_directories('include')
corrupt_private_inc = include_directories('src')

glib_dep = dependency('glib-2.0', version: '>= 2.80.0')
gio_dep = dependency('gio-2.0', version: '>= 2.80.0')

corrupt_lib = static_library('corrupt',
  corrupt_sources,
  include_directories : [corrupt_public_inc, corrupt_private_inc],
  dependencies : [glib_dep, gio_dep])

corrupt_dep = declare_dependency(
  link_with : corrupt_lib,
  include_directories : corrupt_public_inc,
  dependencies : [glib_dep, gio_dep])