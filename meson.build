project('sexplib', 'cpp', version : '0.1', default_options : ['warning_level=3', 'cpp_std=c++20'])

catch2 = dependency('catch2')

sexplib_dep = declare_dependency(include_directories: include_directories('include'))

# Test executable
vectorsexp_test_exe = executable('vectorsexp_test', 'test/vectorsexp_test.cc', dependencies: [catch2, sexplib_dep])
test('VectorSexp parsing tests', vectorsexp_test_exe)

install_headers('include/sexplib.hh', subdir : 'sexplib')
pkg_mod = import('pkgconfig')
pkg_mod.generate(
  name : 'sexplib',
  filebase : 'sexplib',
  description : 'Simple s-expression parsing for C++',
  subdirs : 'sexplib',
  version : '0.1',
)
