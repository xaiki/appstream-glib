# Copyright (C) 2015 Richard Hughes <richard@hughsie.com>
# Copyright (C) 2015 Igor Gnatenko <i.gnatenko.brain@gmail.com>
#
# Licensed under the GNU Lesser General Public License Version 2.1

cargs = ['-DG_LOG_DOMAIN="As"', '-DLOCALSTATEDIR="/var"']

deps = [glib, gdkpixbuf, libarchive, soup, yaml]

asresources = gnome.compile_resources(
  'as-resources', 'appstream-glib.gresource.xml',
  source_dir : '.',
  c_name : 'as')

asversion = configure_file(input : 'as-version.h.in',
                           output : 'as-version.h',
                           configuration : conf)

headers = [
  'appstream-glib.h',
  'as-app.h',
  'as-bundle.h',
  'as-checksum.h',
  'as-enums.h',
  'as-icon.h',
  'as-image.h',
  'as-inf.h',
  'as-node.h',
  'as-problem.h',
  'as-provide.h',
  'as-release.h',
  'as-screenshot.h',
  'as-store.h',
  'as-tag.h',
  'as-utils.h',
  asversion]

sources = [
  'as-app.c',
  'as-app-desktop.c',
  'as-app-inf.c',
  'as-app-private.h',
  'as-app-validate.c',
  'as-bundle.c',
  'as-bundle-private.h',
  'as-checksum.c',
  'as-checksum-private.h',
  'as-cleanup.h',
  'as-enums.c',
  'as-icon.c',
  'as-icon-private.h',
  'as-image.c',
  'as-image-private.h',
  'as-inf.c',
  'as-inf.h',
  'as-monitor.c',
  'as-monitor.h',
  'as-node.c',
  'as-node-private.h',
  'as-problem.c',
  'as-problem.h',
  'as-provide.c',
  'as-provide-private.h',
  'as-release.c',
  'as-release-private.h',
  asresources,
  'as-screenshot.c',
  'as-screenshot-private.h',
  'as-store.c',
  'as-tag.c',
  'as-utils.c',
  'as-utils-private.h',
  'as-yaml.c',
  'as-yaml.h']

if gperf.found()
  astagpriv = custom_target('gperf as-tag',
                            output : 'as-tag-private.h',
                            input : 'as-tag.gperf',
                            command : [gperf, '@INPUT@', '--output-file', '@OUTPUT@'])
  sources = sources + [astagpriv]
endif

install_headers(headers, subdir : 'libappstream-glib')

mapfile = 'appstream-glib.map'
vflag = '-Wl,--version-script,@0@/@1@'.format(meson.current_source_dir(), mapfile)
asglib = shared_library(
  'appstream-glib', sources,
  soversion : lt_current,
  version : lt_version,
  dependencies : deps,
  c_args : cargs,
  include_directories : include_directories('@0@/..'.format(meson.current_build_dir())),
  link_args : ['-Wl,--no-undefined', vflag],
  link_depends : mapfile,
  install : true)
asglib_incdir = include_directories('.')

pkgconfig_gen(
  version : as_version,
  libraries : asglib,
  name : 'appstream-glib',
  description : 'Objects and helper methods to help reading and writing AppStream metadata',
  filebase : 'appstream-glib',
  subddirs : 'appstream-glib')

selftest = executable(
  'as-self-test', 'as-self-test.c',
  include_directories : include_directories('..'),
  dependencies : deps,
  c_args : cargs + ['-DTESTDATADIR="@0@/../data/tests"'.format(meson.current_source_dir())],
  link_with : asglib)
test('as-self-test', selftest)

introspection_sources = [
  'as-app.c',
  'as-app-validate.c',
  'as-app.h',
  'as-bundle.c',
  'as-bundle.h',
  'as-checksum.c',
  'as-checksum.h',
  'as-enums.c',
  'as-enums.h',
  'as-icon.c',
  'as-icon.h',
  'as-image.c',
  'as-image.h',
  'as-inf.c',
  'as-inf.h',
  'as-node.c',
  'as-node.h',
  'as-problem.c',
  'as-problem.h',
  'as-provide.c',
  'as-provide.h',
  'as-release.c',
  'as-release.h',
  'as-screenshot.c',
  'as-screenshot.h',
  'as-store.c',
  'as-store.h',
  'as-tag.c',
  'as-tag.h',
  'as-utils.c',
  'as-utils.h',
  asversion]

gnome.generate_gir(asglib,
  sources : introspection_sources,
  nsversion : '1.0',
  namespace : 'AppStreamGlib',
  symbol_prefix : 'as_',
  identifier_prefix : 'As',
  export_packages : 'appstream-glib',
  includes : ['GObject-2.0', 'Gio-2.0', 'GdkPixbuf-2.0'],
  install : true
)
