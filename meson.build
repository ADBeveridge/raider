project('raider', 'c', version: '2.0.0')

i18n = import('i18n')
gnome = import('gnome')

add_global_arguments('-Ddocs=false',language: 'c')

# Get all dependencies for Raider. Secure Delete should fallback, unless this is being packaged.
raider_deps = [
  #dependency('gio-unix-2.0', version: '>=2.76.0', required: false, fallback : ['glib', 'gio-unix_dep']),
  dependency('gtk4', version: '>=4.12', required : false, default_options: ['media-gstreamer=disabled','build-tests=false','build-testsuite=false'], fallback : ['gtk4', 'gtk4']),
  dependency('libadwaita-1', version: '>= 1.4', required : false, fallback : ['adwaita-build', 'adwaita_lib_dep']),
]

# Create config.h file entirely in Meson.
config_h = configuration_data()
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', 'raider')
config_h.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
configure_file(output: 'raider-config.h', configuration: config_h)
add_project_arguments(['-I' + meson.project_build_root(),], language: 'c')

# Folders with individual meson.build files.
subdir('data')
subdir('src')
subdir('po')

# GNOME specific actions to perform.
gnome.post_install(
  gtk_update_icon_cache: true,
  update_desktop_database: true,
)
