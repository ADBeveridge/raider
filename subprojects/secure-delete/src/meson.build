# Create the config.j file entirely in Meson.
conf_data = configuration_data()
conf_data.set('DIR_SEPERATOR', '\'/\'', description: '/ on Linux, \ on Windows')
conf_data.set('RANDOM_DEVICE', '"/dev/urandom"')
conf_data.set('BLOCKSIZE', '32769', description: 'Must be mod 3 = 0, should be >= 16k')
conf_data.set('FLUSH', 'sync()', description: 'System call to flush the disk')
conf_data.set('MAXINODEWIPE', '4194304', description: '22 bits')
configure_file(output : 'config.h', configuration : conf_data)

# Create the shared library.
securedelete_sources = ['sdel-lib.c']
securedelete = shared_library('sdel',
                        securedelete_sources,
                        include_directories : inc,
                        dependencies : secure_delete_dependencies,
                        install : true)

