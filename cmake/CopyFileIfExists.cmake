if(NOT DEFINED SOURCE OR NOT DEFINED DESTINATION)
  message(FATAL_ERROR "SOURCE and DESTINATION are required")
endif()

if(EXISTS "${SOURCE}")
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${SOURCE}" "${DESTINATION}"
    RESULT_VARIABLE copy_result
  )
  if(copy_result)
    message(FATAL_ERROR "Failed to copy '${SOURCE}' to '${DESTINATION}'")
  endif()
endif()
