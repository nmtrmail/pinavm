# Follows instructions from this site.
# http://llvm.org/docs/CMake.html#embedding-llvm-in-your-project

# the llvm version is setting to this variable.
SET(LLVM_RECOMMAND_VERSION 3.2)

if(NOT DEFINED ${LLVM_ROOT})
  # find llvm-config. perfers to the one with version suffix, Ex:llvm-config-3.2
  find_program(LLVM_CONFIG_EXE NAMES "llvm-config-${LLVM_RECOMMAND_VERSION}" "llvm-config")

  # Get the directory of llvm by using llvm-config. also remove whitespaces.
  execute_process(COMMAND ${LLVM_CONFIG_EXE} --prefix OUTPUT_VARIABLE LLVM_ROOT
		 OUTPUT_STRIP_TRAILING_WHITESPACE )
endif()

# We incorporate the CMake features provided by LLVM:
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${LLVM_ROOT}/share/llvm/cmake")

# Not using version check in find_package is that llvm version is 3.2svn,
# which is different to 3.2 of debian package pool.
# So checks version later.
find_package(LLVM REQUIRED)

# Check whether LLVM 3.2 is found.
if(NOT ${LLVM_FOUND})
  message(FATAL_ERROR "LLVM_ROOT(${LLVM_ROOT}) is not a valid LLVM install\n"
		"You can explicitly specify your llvm_root by "
		"cmake /where/pinavm/is -DLLVM_ROOT=/my/llvm/install/dir\n"
		"or make llvm-config visible in $PATH")
endif()

# Check whether the LLVM version meets our requirement.
# Maybe an ERROR is better than a WARNING message here?
if( NOT ("${LLVM_VERSION_MAJOR}.${LLVM_VERSION_MINOR}" STREQUAL ${LLVM_RECOMMAND_VERSION} ))
  message(WARNING "LLVM version is recommanded to be ${LLVM_RECOMMAND_VERSION}")
endif()

# Use settings from LLVM cmake module.
include_directories( ${LLVM_INCLUDE_DIRS} )
link_directories( ${LLVM_LIBRARY_DIRS} )
add_definitions( ${LLVM_DEFINITIONS} )

llvm_map_components_to_libraries(REQ_LLVM_LIBRARIES jit interpreter nativecodegen bitreader selectiondag asmparser linker)


# Find a compiler which compiles c++ source into llvm bitcode.
# ====================== find CLANG ===================================
find_program(CLANG "clang++-${LLVM_RECOMMAND_VERSION}"
      	  NAMES clang++ 
      	  HINTS ${LLVM_ROOT}/bin /usr/bin )

if(NOT (${CLANG} STREQUAL "CLANG-NOTFOUND"))
  SET(CLANG_FOUND TRUE)
  SET(CLANG_FLAGS -emit-llvm )
  message(STATUS "find ${CLANG}")
else()
  SET(CLANG_FOUND FALSE)
endif()

# ===================== find DragonEgg =================================
# Since DragonEgg is compiled with certain Gcc version, and I do not
# have any good idea to detect the gcc version which DragonEgg use.
# Users should explicitly specify gcc version if they want to use
# DragonEgg.  If users do not specify gcc version, then it is
# meaningless to find DragonEgg.
if(DEFINED DRAGONEGG_GCC)
  # set it to true before finding anything.
  SET(DRAGONEGG_FOUND TRUE)

  # find the dragonegg, the gcc plugin.
  find_library(DRAGONEGG "dragonegg-${LLVM_RECOMMAND_VERSION}.so"
      	  NAMES dragonegg.so
      	  HINTS ${LLVM_ROOT}/lib /usr/lib )
  if(${DRAGONEGG} STREQUAL "DRAGONEGG-NOTFOUND")
    SET(DRAGONEGG_FOUND FALSE)
    message(WARNING "Finds no dragonegg.so")
  endif()

  # User may specify DRAGONEGG_GCC=g++-4.6, but I need absolute path.
  find_program(ABS_DRAGONEGG_GCC ${DRAGONEGG_GCC})
  if(${ABS_DRAGONEGG_GCC} STREQUAL "ABS_DRAGONEGG_GCC-NOFOUND")
    SET(DRAGONEGG_FOUND FALSE)
    message(WARNING "Finds no DRAGONEGG_GCC = \"${DRAGONEGG_GCC}\"")
  endif()

  # if condition true, means CMake finds the dragonegg.so and the correct gcc.
  if(${DRAGONEGG_FOUND})
    SET(DRAGONEGG_FOUND TRUE)
    message(STATUS "find ${DRAGONEGG} with ${ABS_DRAGONEGG_GCC}")
    SET(DRAGONEGG_FLAGS -fplugin=${DRAGONEGG} -fplugin-arg-dragonegg-emit-ir)
  endif()
endif()

# I guess that users perfer DragonEgg if they are willing to take
# a few seconds to specify DRAGONEGG_GCC,
# If users specify nothing(ex: -DDRAGONEGG_GCC=g++-4.6), CMake would not
# find DragonEgg, thus Clang is the only candidate.
if(${DRAGONEGG_FOUND})
  message(STATUS "Emit llvm with DragonEgg")
  SET(LLVM_COMPILER ${ABS_DRAGONEGG_GCC})
  SET(LLVMC_FLAGS ${DRAGONEGG_FLAGS})
else()
  if(${CLANG_FOUND})
    message(STATUS "Emit llvm bitcode with Clang")
    SET(LLVM_COMPILER ${CLANG})
    SET(LLVMC_FLAGS ${CLANG_FLAGS})
  else()
    message(FATAL_ERROR "Could not find either clang or dragonegg."
		" Please install one of them !")
  endif()
endif()



SET(LLVMC_INCLUDE_DIR "-I${CMAKE_SOURCE_DIR}/external/systemc-2.2.0/src/"
                    "-I${CMAKE_SOURCE_DIR}/external/TLM-2009-07-15/include/tlm"
                    "-I${CMAKE_SOURCE_DIR}/external/basic")

# Generate llvm human readable assembly instead of llvm bitcode.
# Quote from DragonEgg site:
 #        Adding -fplugin-arg-dragonegg-emit-ir or -flto causes LLVM IR
 #        to be output (you need to request assembler output, -S, rather
 #        than object code output, -c, since otherwise gcc will pass the
 #        LLVM IR to the system assembler, which will doubtless fail to
 #        assemble it):

SET(LLVMC_FLAGS ${LLVMC_FLAGS} ${LLVM_DEFINITIONS} ${LLVMC_INCLUDE_DIR}
		-fno-inline-functions -fno-use-cxa-atexit -S )

# For debug use only
if(false)
  message("debug messages below")
  message("LLVM_ROOT is ${LLVM_ROOT}")
  message("REQ_LLVM_LIBRARIES is ${REQ_LLVM_LIBRARIES}")
  message("LLVM_COMPILER is ${LLVM_COMPILER}")
  message("LLVM_DEFINITIONS is ${LLVM_DEFINITIONS}")
  message("LLVM_LIBRARY_DIRS is ${LLVM_LIBRARY_DIRS}")
  message("LLVM_INCLUDE_DIRS is ${LLVM_INCLUDE_DIRS}")
endif()