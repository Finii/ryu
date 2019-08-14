# Ryu library only, no Ryu printf

project('ryu', ['cpp', 'c'], meson_version : '>=0.40.0')

srcs = [
    'f2s.c',
    'd2s.c',
]

install_headers(files('ryu.h'))

strange_base_include_dir = include_directories('..')

libryu = library('ryu', srcs,
    include_directories : strange_base_include_dir,
    install : true)

subdir('tests')

example = executable('example', 'example.cc',
    link_with : libryu )

