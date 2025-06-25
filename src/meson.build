# Define dependencies
pipewire_dep = dependency('libpipewire-0.3', required : true)
liblo_dep = dependency('liblo', required : true)
libsndfile_dep = dependency('sndfile', required : false)
libmicrohttpd_dep = dependency('libmicrohttpd', required : false)

# Add all source files for static linking
srcs = [
  'pw-ghost-rec.c',
  'audio-buffer.c',
  'channel-buffer.c',
  'ring-buffer.c',
]

# Define the executable and link dependencies
executable('pw-ghost-rec', srcs,
  dependencies: [pipewire_dep, liblo_dep, libsndfile_dep, libmicrohttpd_dep],
  c_args: ['-O2', '-Wno-pedantic'],
  install: true,
  install_dir: get_option('bindir'),
)
