execute_process(COMMAND ${TEST_EXE} --list OUTPUT_VARIABLE tests RESULT_VARIABLE result)

if (NOT result EQUAL 0)
	message(FATAL_ERROR "--list failed (${result}): ${tests}")
endif()

string(REPLACE "\r" "" tests "${tests}")
string(REPLACE "\n" ";" tests "${tests}")

file(WRITE "${OUT_FILE}" "")
foreach (test_name ${tests})
	if ("${test_name}" MATCHES "^selftest/")
		continue()
	endif()

	string(FIND "${test_name}" "/" split_index)
	string(SUBSTRING "${test_name}" 0 "${split_index}" suite)

	file(APPEND "${OUT_FILE}" "add_test([=[${test_name}]=] [=[${TEST_EXE}]=] --test [=[${test_name}]=])\n")
	file(APPEND "${OUT_FILE}" "set_tests_properties([=[${test_name}]=] PROPERTIES TIMEOUT 30 LABELS [=[${suite}]=] WORKING_DIRECTORY [=[${SRC_DIR}]=])\n")
endforeach()