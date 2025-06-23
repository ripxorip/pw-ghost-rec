# Define dependencies
pipewire_dep = dependency('libpipewire-0.3', required : true)
liblo_dep = dependency('liblo', required : true)
libsndfile_dep = dependency('sndfile', required : false)

# Define the executable and link dependencies
executable('pw-ghost-rec', 'pw-ghost-rec.c',
  dependencies: [pipewire_dep, liblo_dep, libsndfile_dep],
  c_args: ['-O2', '-Wno-pedantic']
)
