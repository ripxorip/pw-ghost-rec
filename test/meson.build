# Test meson.build for pw-ghost-rec

dep_check = dependency('check')

src = ['test_ring_buffer.c', '../src/ring-buffer.c']

test_ring_buffer_exe = executable('test_ring_buffer', src,
  include_directories: include_directories('..', '../src'),
  dependencies: dep_check,
  install: false
)

test('ring_buffer', test_ring_buffer_exe,
  env: environment(),
)
