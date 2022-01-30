shared_module('nautilus-raider', 
  sources : [
    'nautilus-raider.c'
  ],
  dependencies : [ 
    dependency('glib-2.0', version : '>=2.38'), 
    libnautilus_extension_dep
  ],  
  install : true,
  install_dir : libnautilus_extension_dep.get_pkgconfig_variable('extensiondir')
)
