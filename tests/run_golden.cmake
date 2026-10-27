# Golden-file test: runs `CMD SUBCMD INPUT` and compares its stdout with EXPECTED.
#
#   cmake -DCMD=<rsharp exe> -DSUBCMD=lex -DINPUT=<file.rsh> -DEXPECTED=<golden file>
#         [-DEXPECT_EXIT=0] [-DUPDATE=1] -P run_golden.cmake
#
# Line endings are normalized on both sides so CRLF checkouts still match.
# Pass -DUPDATE=1 to rewrite the golden file from the current output instead.

if(NOT DEFINED EXPECT_EXIT)
  set(EXPECT_EXIT 0)
endif()

execute_process(
  COMMAND "${CMD}" "${SUBCMD}" "${INPUT}"
  OUTPUT_VARIABLE actual
  ERROR_VARIABLE stderr
  RESULT_VARIABLE exit_code)
string(REPLACE "\r\n" "\n" actual "${actual}")

if(UPDATE)
  file(WRITE "${EXPECTED}" "${actual}")
  message(STATUS "updated ${EXPECTED}")
  return()
endif()

if(NOT exit_code STREQUAL "${EXPECT_EXIT}")
  message(FATAL_ERROR "expected exit code ${EXPECT_EXIT}, got ${exit_code}\nstderr:\n${stderr}")
endif()

file(READ "${EXPECTED}" expected)
string(REPLACE "\r\n" "\n" expected "${expected}")

if(NOT actual STREQUAL expected)
  message(FATAL_ERROR "output differs from ${EXPECTED}\n--- expected\n${expected}\n--- actual\n${actual}\n"
                      "(rerun with -DUPDATE=1 to accept the new output)")
endif()
