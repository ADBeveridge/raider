project('raider', 'c', version: '3.1.0', default_options : ['c_std=c23'])

i18n = import('i18n')
gnome = import('gnome')

glib_dep = dependency('glib-2.0', version: '>= 2.80.0')
gtk_dep = dependency('gtk4', version: '>= 4.20.0')
libadwaita_dep = dependency('libadwaita-1', version: '>= 1.8.0')
blueprint_dep = dependency('blueprint-compiler', native: true, version: '>= 0.10.0')

raider_deps = [gtk_dep, glib_dep, libadwaita_dep, blueprint_dep]

# Create config.h file entirely in Meson.
config_h = configuration_data()
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', 'raider')
config_h.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
configure_file(output: 'raider-config.h', configuration: config_h)
add_project_arguments(['-I' + meson.project_build_root(),], language: 'c')

# Folders with individual meson.build files.
subdir('corrupt')
subdir('data')
subdir('src')
subdir('po')

# GNOME actions to perform.
gnome.post_install(
  gtk_update_icon_cache: true,
  update_desktop_database: true,
)
