project('sexplib', 'cpp', version : '0.1', default_options : ['warning_level=3', 'cpp_std=c++20'])

if get_option('buildtype') == 'release'
  add_project_arguments('-march=native', language: 'cpp')
  add_project_arguments('-DNDEBUG', language: 'cpp')
endif

catch2 = dependency('catch2')
fmt = dependency('fmt', required: false)

optional_fmt_define = ''
if fmt.found()
  optional_fmt_define = '-DSEXPLIB_USE_FMT'
endif

sexplib_dep = declare_dependency(
  compile_args: optional_fmt_define,
  include_directories: include_directories('include')
)

# Test executables
# vectorsexp_test_exe = executable('vectorsexp_test', 'test/vectorsexp_test.cc', dependencies: [catch2, sexplib_dep])
# test('VectorSexp parsing tests', vectorsexp_test_exe)
# sexp_test_exe = executable('sexp_test', 'test/sexp_test.cc', dependencies: [catch2, sexplib_dep])
# test('Sexp parsing tests', sexp_test_exe)

install_headers('include/sexplib.hh', subdir : 'sexplib')
pkg_mod = import('pkgconfig')
pkg_mod.generate(
  name : 'sexplib',
  filebase : 'sexplib',
  description : 'Simple s-expression parsing for C++',
  subdirs : 'sexplib',
  version : '0.1',
)
