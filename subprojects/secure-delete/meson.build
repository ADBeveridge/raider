project('secure-delete', 'c')

secure_delete_dependencies = dependency('glib-2.0')

inc = include_directories('include')
subdir('include')
subdir('src')

# Generate pkgconfig library.
pkg_mod = import('pkgconfig')
pkg_mod.generate(libraries : securedelete,
                 version : '1.0',
                 name : 'libsecure-delete',
                 filebase : 'foobar',
                 description : 'A library to shred files.')

