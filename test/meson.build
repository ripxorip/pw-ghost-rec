# Test meson.build for pw-ghost-rec

dep_check = dependency('check')
libsndfile_dep = dependency('sndfile')

src = ['test_ring_buffer.c', '../src/ring-buffer.c']

channel_src = ['test_channel_buffer.c', '../src/channel-buffer.c', '../src/ring-buffer.c']
audio_src = ['test_audio_buffer.c', '../src/audio-buffer.c', '../src/channel-buffer.c', '../src/ring-buffer.c']

test_ring_buffer_exe = executable('test_ring_buffer', src,
  include_directories: include_directories('..', '../src'),
  dependencies: [dep_check, libsndfile_dep],
  link_args: ['-lm'],
  c_args: ['-O2', '-Wno-pedantic', '-Wno-gnu-statement-expression'],
  install: false
)

test_channel_buffer_exe = executable('test_channel_buffer', channel_src,
  include_directories: include_directories('..', '../src'),
  dependencies: [dep_check, libsndfile_dep],
  link_args: ['-lm'],
  c_args: ['-O2', '-Wno-pedantic', '-Wno-gnu-statement-expression'],
  install: false
)

test_audio_buffer_exe = executable('test_audio_buffer', audio_src,
  include_directories: include_directories('..', '../src'),
  dependencies: [dep_check, libsndfile_dep],
  link_args: ['-lm'],
  c_args: ['-O2', '-Wno-pedantic', '-Wno-gnu-statement-expression'],
  install: false
)

test('ring_buffer', test_ring_buffer_exe,
  env: environment(),
)
test('channel_buffer', test_channel_buffer_exe,
  env: environment(),
)
test('audio_buffer', test_audio_buffer_exe,
  env: environment(),
)
