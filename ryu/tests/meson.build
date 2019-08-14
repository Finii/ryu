test_src = [
    'd2s_intrinsics_test.cc',
    'd2s_table_test.cc',
    'd2s_test.cc',
    'f2s_test.cc',
    'd2fixed_test.cc',
    'generic_128_test.cc',
]

# We need d2fixed for the d2fixed test ;-)
# too lazy to build ryu2
test_src += [ '../d2fixed.c' ]

# We need generic_128 for the generic_128 test ;-)
test_src += [ '../generic_128.c' ]

testryu = executable('testryu', test_src,
    include_directories : strange_base_include_dir,
    link_with : libryu )

test('all', testryu)
