# Unit tests use a tiny built-in harness (no external dependency yet).
# We can switch to Catch2/GoogleTest later via FetchContent if wanted.

add_executable(rsharp_tests test_main.cpp)
target_link_libraries(rsharp_tests PRIVATE rsharp_core)
add_test(NAME rsharp_unit COMMAND rsharp_tests)

# CLI smoke test
add_test(NAME cli_version COMMAND rsharp version)
set_tests_properties(cli_version PROPERTIES PASS_REGULAR_EXPRESSION "rsharp 0\\.[0-9]+")
