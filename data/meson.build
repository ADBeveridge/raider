desktop_file = i18n.merge_file(
  input: 'com.github.ADBeveridge.Raider.desktop.in.in',
  output: 'com.github.ADBeveridge.Raider.desktop',
  type: 'desktop',
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'applications')
)

desktop_utils = find_program('desktop-file-validate', required: false)
if desktop_utils.found()
  test('Validate desktop file', desktop_utils,
    args: [desktop_file]
  )
endif

appstream_file = i18n.merge_file(
  input: 'com.github.ADBeveridge.Raider.metainfo.xml.in.in',
  output: 'com.github.ADBeveridge.Raider.metainfo.xml',
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'metainfo')
)

appstreamcli = find_program('appstreamcli', required: false)
if appstreamcli.found()
  test('Validate appstream file', appstreamcli,
    args: ['validate', '--no-net', appstream_file]
  )
endif

subdir('icons')
