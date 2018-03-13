# Written by Seungbin Song

set (MULTIP4_SRCS
  p4c-multip4.cpp
  tableAnalyzer.cpp
  )

set (MULTIP4_HDRS
  tableAnalyzer.h
  )

add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${MULTIP4_SRCS};${MULTIP4_HDRS}")

build_unified(MULTIP4_SRCS ALL)
add_executable(p4c-multip4 ${MULTIP4_SRCS})
target_link_libraries (p4c-multip4 ${P4C_LIBRARIES} ${P4C_LIB_DEPS})

install (TARGETS p4c-multip4
  RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})
