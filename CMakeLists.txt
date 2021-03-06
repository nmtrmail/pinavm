cmake_minimum_required(VERSION 2.8.12)

project(PinaVM CXX)
enable_testing()

include(${CMAKE_SOURCE_DIR}/scripts/PinaVMConfig.cmake)

# Find and set LLVM.
include(${CMAKE_SOURCE_DIR}/scripts/findAndSetLLVM.cmake)

# Some files define classes that inherit from LLVM classes, which are
# compiled with -fno-rtti as of svn version 98434 (14 mach 2010) while
# we do need -frtti (see above). Deriving a rtti class from a no-rtti
# doesn't link. So, these files are compiled without rtti.
macro(needs_no_rtti FILE)
  SET_SOURCE_FILES_PROPERTIES(${FILE} PROPERTIES COMPILE_FLAGS "-fno-rtti -fno-exceptions") 
endmacro()

# subdirectories
include_directories(${CMAKE_SOURCE_DIR}/external/systemc-${SYSTEMC_VERSION}/src)
include_directories(${CMAKE_SOURCE_DIR}/external/basic/)
include_directories(${CMAKE_SOURCE_DIR}/utils)

include(${CMAKE_SOURCE_DIR}/scripts/systemc.cmake)
add_subdirectory(toplevel)
add_subdirectory(frontend)
add_subdirectory(utils)
add_subdirectory(external/basic)

# add backends
foreach(backend ${BACKENDS})
  add_subdirectory(backends/${backend}Backend/)
endforeach(backend)

add_subdirectory(systemc-examples)

string(REPLACE ";" " " LLVMC_FLAGS_SPACE "${LLVMC_FLAGS}")
configure_file(config.sh.in config.sh)

