MACRO(decide_patch_version LLVM_RECOMMEND_VERSION_ARG)
  # force to use newest patch version
  if (${LLVM_RECOMMEND_VERSION_ARG} VERSION_LESS "3.4")
    SET(LLVM_PATCH_VERSION ${LLVM_RECOMMEND_VERSION_ARG})
    SET(CLANG_PATCH_VERSION ${LLVM_RECOMMEND_VERSION_ARG})
    SET(RT_PATCH_VERSION ${LLVM_RECOMMEND_VERSION_ARG})
  elseif (${LLVM_RECOMMEND_VERSION_ARG} VERSION_EQUAL "3.4")
    SET(LLVM_PATCH_VERSION "3.4.2")
    SET(CLANG_PATCH_VERSION "3.4.2")
    SET(RT_PATCH_VERSION "3.4")
  elseif (${LLVM_RECOMMEND_VERSION_ARG} VERSION_EQUAL "3.5")
    SET(LLVM_PATCH_VERSION "3.5.2")
    SET(CLANG_PATCH_VERSION "3.5.2")
    SET(RT_PATCH_VERSION "3.5.2")
  elseif (${LLVM_RECOMMEND_VERSION_ARG} VERSION_EQUAL "3.6")
    SET(LLVM_PATCH_VERSION "3.6.2")
    SET(CLANG_PATCH_VERSION "3.6.2")
    SET(RT_PATCH_VERSION "3.6.2")
  else()
    message(FATAL_ERROR "Unsupported LLVM version : ${LLVM_RECOMMEND_VERSION_ARG}")
  endif()
ENDMACRO()
