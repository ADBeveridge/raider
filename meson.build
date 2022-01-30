project('raider', 'c',        
	version: '0.1.0',
  	meson_version: '>= 0.40.0',
)

i18n = import('i18n')

# Optional deps
nautilus_version = '>=3.28.0'

if get_option('nautilus-actions')
  libnautilus_extension_dep = dependency('libnautilus-extension', version : nautilus_version, required : false)
  build_nautilus_actions = libnautilus_extension_dep.found()
else
  build_nautilus_actions = false
endif

config_h = configuration_data()
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', 'raider')
config_h.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
configure_file(
  output: 'raider-config.h',
  configuration: config_h,
)
add_project_arguments([
  '-I' + meson.build_root(),
], language: 'c')

prefix = get_option('prefix')
datadir = join_paths(prefix, get_option('datadir'))

subdir('data')
subdir('src')
subdir('po')
subdir('docs')
subdir('help')
if build_nautilus_actions
  subdir('nautilus')
endif

meson.add_install_script('build-aux/meson/postinstall.py')
