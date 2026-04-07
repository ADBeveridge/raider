corrupt_sources = [
  'src/corrupt.c',
  'src/bucket.c',
  'src/utility.c'
]

corrupt_public_inc = include_directories('include')
corrupt_private_inc = include_directories('src')

thread_dep = dependency('threads')

corrupt_lib = static_library('corrupt',
  corrupt_sources,
  include_directories : [corrupt_public_inc, corrupt_private_inc],
  dependencies : thread_dep)

corrupt_dep = declare_dependency(
  link_with : corrupt_lib,
  include_directories : corrupt_public_inc,
  dependencies : thread_dep)