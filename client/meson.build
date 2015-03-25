as_util_cargs = ['-DG_LOG_DOMAIN="As"']

appstream_util_srcs = [
  'as-util.c'
]

appstream_util = executable(
  'appstream-util',
  sources : appstream_util_srcs,
  include_directories : [
    include_directories('@0@/..'.format(meson.current_build_dir())),
    asglib_incdir,
  ],
  dependencies : [glib, gdkpixbuf, soup, libarchive],
  link_with : asglib,
  c_args : as_util_cargs
)
