# Copyright (C) 2015 Igor Gnatenko <i.gnatenko.brain@gmail.com>
#
# Licensed under the GNU Lesser General Public License Version 2.1

asb_plugins_cargs = ['-DG_LOG_DOMAIN="Asb"']

asb_plugins = [
  ['dbus',         'asb-plugin-dbus.c',         []],
  ['firmware',     'asb-plugin-firmware.c',     []],
  ['absorb',       'asb-plugin-absorb.c',       []],
  ['kde_services', 'asb-plugin-kde-services.c', []],
  ['kde_notifyrc', 'asb-plugin-kde-notifyrc.c', []],
  ['gstreamer',    'asb-plugin-gstreamer.c',    []],
  ['metainfo',     'asb-plugin-metainfo.c',     []],
  ['gettext',      'asb-plugin-gettext.c',      []],
  ['blacklist',    'asb-plugin-blacklist.c',    []],
  ['hardcoded',    'asb-plugin-hardcoded.c',    []],
  ['desktop',      'asb-plugin-desktop.c',      []],
  ['gir',          'asb-plugin-gir.c',          []],
  ['gresource',    'asb-plugin-gresource.c',    []],
  ['appdata',      'asb-plugin-appdata.c',      []],
  ['nm',           'asb-plugin-nm.c',           []],
  ['ibus_sql',     'asb-plugin-ibus-sql.c',     []],
  ['ibus_xml',     'asb-plugin-ibus-xml.c',     []],
  ['font',         'asb-plugin-font.c',         [gdk]],
]

foreach i : asb_plugins
  shared_library(
    'libasb_plugin_@0@'.format(i.get(0)),
    sources : i.get(1),
    include_directories: [
      top_build_incdir,
      asbuilder_incdir,
      asglib_incdir,
    ],
    dependencies : [
      glib, gdkpixbuf, soup,
      i.get(2),
    ],
    c_args : asb_plugins_cargs
  )
endforeach
