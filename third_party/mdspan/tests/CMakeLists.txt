
#The tests in CUDA use lambdas
if(MDSPAN_ENABLE_CUDA)
  if(CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --extended-lambda")
  endif()
endif()

function(mdspan_add_test name)
  set(options ENABLE_PRECONDITIONS)
  set(one_value SOURCE)
  set(multi_value)
  cmake_parse_arguments(ARGUMENT "${options}" "${one_value}" "${multi_value}" ${ARGN})

  if(NOT DEFINED ARGUMENT_SOURCE)
    set(ARGUMENT_SOURCE ${name}.cpp)
  endif()

  if(MDSPAN_TEST_LANGUAGE)
    set_source_files_properties(${name} PROPERTIES LANGUAGE ${MDSPAN_TEST_LANGUAGE})
  endif()
  add_executable(${name} ${ARGUMENT_SOURCE})
  target_link_libraries(${name} mdspan gtest_main)
  add_test(${name} ${name})
  set_property(TARGET ${name} PROPERTY COMPILE_WARNING_AS_ERROR ON)
  target_compile_definitions(${name}
    PUBLIC
    MDSPAN_IMPL_CHECK_PRECONDITION=$<BOOL:${ARGUMENT_ENABLE_PRECONDITIONS}>
  )
endfunction()

if(MDSPAN_USE_SYSTEM_GTEST)
  find_package(GTest CONFIG REQUIRED)
  add_library(gtest_main ALIAS GTest::gtest_main)
else()
  # adapted from https://github.com/google/googletest/blob/master/googletest/README.md
  configure_file(${PROJECT_SOURCE_DIR}/cmake/googletest/CMakeLists.txt.in googletest-download/CMakeLists.txt)
  execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download
  )
  if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
  endif()
  execute_process(COMMAND ${CMAKE_COMMAND} --build .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download
  )
  if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
  endif()

  # Prevent overriding the parent project's compiler/linker
  # settings on Windows
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

  # Add googletest directly to our build. This defines
  # the gtest and gtest_main targets.
  add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
    ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
    EXCLUDE_FROM_ALL
  )
endif()

# FIXME_MSVC
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
mdspan_add_test(
  test_precondition_checks_can_be_disabled
  SOURCE test_alternate_precondition_violation_handler.cpp
)
target_compile_definitions(
  test_precondition_checks_can_be_disabled
PUBLIC
  MDSPAN_IMPL_CHECK_PRECONDITION=0
)
set_tests_properties(
  test_precondition_checks_can_be_disabled
PROPERTIES
  PASS_REGULAR_EXPRESSION "Failure.*Expected.*throws an exception of type std::logic_error.*Actual.*it throws nothing"
)
endif()

if(NOT MDSPAN_ENABLE_CUDA AND NOT MDSPAN_ENABLE_HIP AND NOT MDSPAN_ENABLE_SYCL)
mdspan_add_test(test_alternate_precondition_violation_handler ENABLE_PRECONDITIONS)
endif()
mdspan_add_test(test_macros ENABLE_PRECONDITIONS)
mdspan_add_test(test_layout_preconditions ENABLE_PRECONDITIONS)

mdspan_add_test(test_dims)
mdspan_add_test(test_extents)
mdspan_add_test(test_mdspan_ctors)
mdspan_add_test(test_mdspan_swap)
mdspan_add_test(test_mdspan_conversion)
mdspan_add_test(test_mdspan_size)
mdspan_add_test(test_element_access)
mdspan_add_test(test_exhaustive_layouts)
mdspan_add_test(test_layout_ctors)
mdspan_add_test(test_layout_stride)
if(NOT CMAKE_CXX_STANDARD STREQUAL "14")
  mdspan_add_test(test_submdspan)
  mdspan_add_test(test_submdspan_static_slice)
  mdspan_add_test(test_layout_padded_left)
  mdspan_add_test(test_layout_padded_right)
endif()
# both of those don't work yet since its using vector
if(NOT MDSPAN_ENABLE_CUDA AND NOT MDSPAN_ENABLE_HIP)
mdspan_add_test(test_mdarray_ctors)
mdspan_add_test(test_mdarray_to_mdspan)
endif()

if(CMAKE_CXX_STANDARD GREATER_EQUAL 20)
if((CMAKE_CXX_COMPILER_ID STREQUAL Clang) OR ((CMAKE_CXX_COMPILER_ID STREQUAL GNU) AND (CMAKE_CXX_COMPILER_VERSION GREATER_EQUAL 12.0.0)))
  add_subdirectory(libcxx-backports)
endif()
endif()
