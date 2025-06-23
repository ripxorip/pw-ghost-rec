# Test meson.build for pw-ghost-rec

dep_check = dependency('check')

src = ['test_ring_buffer.c', '../src/ring-buffer.c']

test_ring_buffer_exe = executable('test_ring_buffer', src,
  include_directories: include_directories('..', '../src'),
  dependencies: dep_check,
  link_args: ['-lm'],
  c_args: ['-O2', '-Wno-pedantic', '-Wno-gnu-statement-expression'],
  install: false
)

test('ring_buffer', test_ring_buffer_exe,
  env: environment(),
)
