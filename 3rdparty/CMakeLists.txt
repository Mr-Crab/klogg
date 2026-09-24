set(QAPPLICATION_CLASS QApplication)

include(CPM)

function(klogg_patch_dependency_file file_path old_text new_text)
  file(READ "${file_path}" file_contents)
  string(REPLACE "\r\n" "\n" file_contents "${file_contents}")
  string(REPLACE "${old_text}" "${new_text}" patched_contents "${file_contents}")
  if(NOT patched_contents STREQUAL file_contents)
    file(WRITE "${file_path}" "${patched_contents}")
  endif()
endfunction()

set(_TMP_CPM_USE_LOCAL_PACKAGES ${CPM_USE_LOCAL_PACKAGES})

cpmaddpackage("gh:simdutf/simdutf@9.1.0")
if(TARGET simdutf)
  # simdutf's own src/CMakeLists.txt hardcodes /WX /W3 /sdl for any MSVC-like
  # compiler that isn't the old toolset 140. clang-cl also reports as MSVC and
  # falls into that branch, but unlike real cl.exe it emits -Wunused-template
  # for simdutf's per-backend scalar fallbacks, which then turns fatal under
  # /WX. Silence it; this is appended after simdutf's own target_compile_options
  # call so it wins the flag-ordering fight against its /W3.
  target_compile_options(simdutf PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wno-unused-template>)
endif()
cpmaddpackage("gh:RoaringBitmap/CRoaring@5.1.1")
set(ROARING_BUILD_CPP ON CACHE BOOL "Build the C++ wrapper" FORCE)
cpmaddpackage(
  NAME
  streamvbyte
  GITHUB_REPOSITORY
  lemire/streamvbyte
  VERSION
  3.0.0
)
if(streamvbyte_ADDED)
  klogg_patch_dependency_file(
    "${streamvbyte_SOURCE_DIR}/src/streamvbyte_isadetection.h"
    [=[#if defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC)
#define STREAMVBYTE_IS_ARM64 1
#endif // defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC)
#if defined(_MSC_VER)]=]
    [=[/*
 * The ARM SIMD sources use GCC/Clang AArch64 intrinsics. MSVC ARM64 uses a
 * different intrinsic surface, so keep its portable scalar implementation.
 */
#if defined(__aarch64__) && !defined(_MSC_VER)
#define STREAMVBYTE_IS_ARM64 1
#endif

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))]=]
  )
endif()

if(APPLE)
  cpmaddpackage(
    NAME
    macdeployqtfix
    GITHUB_REPOSITORY
    arl/macdeployqtfix
    GIT_TAG
    631361abd994640023c83b539735d8373744f9dd
    DOWNLOAD_ONLY
    YES
  )
  if(macdeployqtfix_ADDED)
    configure_file(${macdeployqtfix_SOURCE_DIR}/macdeployqtfix.py ${CMAKE_BINARY_DIR}/macdeployqtfix.py COPYONLY)
  endif()
endif()

cpmaddpackage(
  NAME
  maddy
  GITHUB_REPOSITORY
  variar/maddy
  GIT_TAG
  602e26613e624535e2de883b3f2c98a16729d1d4
  EXCLUDE_FROM_ALL
  YES
)

if(KLOGG_USE_HYPERSCAN)
  cpmaddpackage(
    NAME
    hyperscan
    GITHUB_REPOSITORY
    variar/hyperscan
    GIT_TAG
    0931a40e0cf1d7f92189bc546c3491ed5c113f8b
    EXCLUDE_FROM_ALL
    YES
  )
  message("Adding alias for hyperscan")
  add_library(klogg_hyperscan INTERFACE)
  target_link_libraries(klogg_hyperscan INTERFACE hs)
  target_include_directories(klogg_hyperscan INTERFACE ${hyperscan_SOURCE_DIR}/src)
elseif(KLOGG_USE_VECTORSCAN)
  cpmaddpackage(
    NAME
    vectorscan
    GITHUB_REPOSITORY
    VectorCamp/vectorscan
    GIT_TAG
    acd7363aadea43da9c5246542d9969db843dd132
    EXCLUDE_FROM_ALL
    YES
    OPTIONS
    "BUILD_STATIC_LIBS ON"
    "BUILD_UNIT OFF"
    "BUILD_TOOLS OFF"
    "BUILD_EXAMPLES OFF"
    "BUILD_BENCHMARKS OFF"
    "BUILD_DOC OFF"
    "BUILD_CHIMERA OFF"
    "BUIlD_AVX2 OFF"
    "BUIlD_AVX512 OFF"
    "BUIlD_AVX512VBMI OFF"
    "FAT_RUNTIME OFF"
  )
 
  message("Adding alias for vectorscan")
  add_library(klogg_vectorscan INTERFACE)
  target_link_libraries(klogg_vectorscan INTERFACE hs)
  target_include_directories(klogg_vectorscan INTERFACE 
    ${vectorscan_SOURCE_DIR}/src
    ${vectorscan_BINARY_DIR}
  )
endif()

cpmaddpackage(
  NAME
  Uchardet
  GIT_REPOSITORY
  https://gitlab.freedesktop.org/uchardet/uchardet
  VERSION
  0.0.8
  EXCLUDE_FROM_ALL
  YES
  OPTIONS
  "BUILD_BINARY OFF"
)
if(Uchardet_ADDED)
  message("Adding alias for uchardet")
  add_library(klogg_uchardet_wrapper INTERFACE)
  target_link_libraries(klogg_uchardet_wrapper INTERFACE libuchardet)
  target_include_directories(klogg_uchardet_wrapper INTERFACE ${Uchardet_SOURCE_DIR}/src)
else()
  add_library(klogg_uchardet_wrapper INTERFACE)
  target_link_libraries(klogg_uchardet_wrapper INTERFACE ${UCHARDET_LIBRARY})
  target_include_directories(klogg_uchardet_wrapper INTERFACE ${UCHARDET_INCLUDE_DIR})
endif()

set(CPM_USE_LOCAL_PACKAGES OFF)

# TODO: Need replacement in the future
cpmaddpackage(
  NAME
  KF5Archive
  GITHUB_REPOSITORY
  variar/klogg_karchive
  GIT_TAG
  f546bf6ae66a8d34b43da5a41afcfbf4e1a47906
  EXCLUDE_FROM_ALL
  YES
)
if(KF5Archive_ADDED)
  klogg_patch_dependency_file(
    "${KF5Archive_SOURCE_DIR}/karchive/src/kar.cpp"
    ".arg(mode)"
    ".arg(static_cast<int>(mode))"
  )
  klogg_patch_dependency_file(
    "${KF5Archive_SOURCE_DIR}/karchive/src/karchive.cpp"
    ".arg(mode)"
    ".arg(static_cast<int>(mode))"
  )
  klogg_patch_dependency_file(
    "${KF5Archive_SOURCE_DIR}/karchive/src/karchive.cpp"
    ".arg(d->mode)"
    ".arg(static_cast<int>(d->mode))"
  )
  klogg_patch_dependency_file(
    "${KF5Archive_SOURCE_DIR}/karchive/src/krcc.cpp"
    ".arg(mode)"
    ".arg(static_cast<int>(mode))"
  )
endif()
if(NOT KF5Archive_ADDED)
  find_package(KF5Archive)
  add_library(klogg_karchive INTERFACE)
  target_link_libraries(klogg_karchive INTERFACE KF5::Archive)
endif()

message("Resetting CPM_USE_LOCAL_PACKAGES to ${_TMP_CPM_USE_LOCAL_PACKAGES}")
set(CPM_USE_LOCAL_PACKAGES ${_TMP_CPM_USE_LOCAL_PACKAGES})

cpmaddpackage(
  NAME
  robin_hood
  GITHUB_REPOSITORY
  martinus/robin-hood-hashing
  GIT_TAG
  3.11.5
  EXCLUDE_FROM_ALL
  YES
)
if(NOT TARGET robin_hood)
  message("Adding imported target for robin_hood")
  add_library(robin_hood INTERFACE)
  target_include_directories(robin_hood INTERFACE ${ROBIN_HOOD_INCLUDE_DIRS})
endif(NOT TARGET robin_hood)

if(KLOGG_BUILD_TESTS)
  cpmaddpackage("gh:bombela/backward-cpp@1.6")
  cpmaddpackage(
    NAME
    Catch2
    GITHUB_REPOSITORY
    catchorg/Catch2
    VERSION
    2.13.8
    EXCLUDE_FROM_ALL
    YES
  )
  if(NOT TARGET Catch2)
    message("Adding imported target for catch2")
    add_library(Catch2 INTERFACE)
    target_include_directories(Catch2 INTERFACE ${CATCH2_INCLUDE_DIRS})
  endif(NOT TARGET Catch2)
endif()

cpmaddpackage(
  NAME
  KDSingleApplication
  GITHUB_REPOSITORY
  KDAB/KDSingleApplication
  GIT_TAG
  v1.2.1
  EXCLUDE_FROM_ALL
  YES
  OPTIONS
  "KDSingleApplication_STATIC ON"
  "KDSingleApplication_TESTS OFF"
  "KDSingleApplication_EXAMPLES OFF"
  "KDSingleApplication_DOCS OFF"
)
if(KDSingleApplication_ADDED)
  # Official target name is lowercase 'kdsingleapplication'.
  # Alias is KDAB::kdsingleapplication (also lowercase).
  add_library(kdsingleapp ALIAS kdsingleapplication)
endif()


cpmaddpackage(
  NAME
  xxHash
  GITHUB_REPOSITORY
  Cyan4973/xxHash
  VERSION
  0.8.3
  DOWNLOAD_ONLY
  YES
)
if(xxHash_ADDED)
  set(XXHASH_BUILD_ENABLE_INLINE_API OFF)
  set(XXHASH_BUILD_XXHSUM OFF)
  add_subdirectory(${xxHash_SOURCE_DIR}/cmake_unofficial ${CMAKE_BINARY_DIR}/xxhash EXCLUDE_FROM_ALL)
endif()

cpmaddpackage(
  NAME
  whereami
  GITHUB_REPOSITORY
  gpakosz/whereami
  GIT_TAG
  dcb52a058dc14530ba9ae05e4339bd3ddfae0e0e
  DOWNLOAD_ONLY
  YES
)
if(whereami_ADDED)
  add_library(whereami STATIC ${whereami_SOURCE_DIR}/src/whereami.h ${whereami_SOURCE_DIR}/src/whereami.c)
  target_include_directories(whereami PUBLIC ${whereami_SOURCE_DIR}/src)
endif()

cpmaddpackage(
  NAME
  exprtk
  GITHUB_REPOSITORY
  variar/klogg_exprtk
  GIT_TAG
  1f9f4cd7d2620b7b24232de9ea22908d63913459
  DOWNLOAD_ONLY
  YES
)
if(exprtk_ADDED)
  add_library(exprtk INTERFACE)
  target_link_libraries(exprtk INTERFACE robin_hood)
  target_include_directories(exprtk SYSTEM INTERFACE ${exprtk_SOURCE_DIR})
  target_compile_definitions(
    exprtk
    INTERFACE -Dexprtk_disable_caseinsensitivity
              -Dexprtk_disable_comments
              -Dexprtk_disable_break_continue
              -Dexprtk_disable_return_statement
              -Dexprtk_disable_superscalar_unroll
              -Dexprtk_disable_rtl_io_file
              -Dexprtk_disable_rtl_vecops
              -Dexprtk_disable_string_capabilities
  )
endif()

cpmaddpackage(
  NAME
  KDToolBox
  GITHUB_REPOSITORY
  KDABLabs/KDToolBox
  GIT_TAG
  39ea6fae6b7edf4d7f31c3a4ee7202c619ce8392
  DOWNLOAD_ONLY
  YES
)
if(KDToolBox_ADDED)
  add_library(
    kdtoolbox STATIC ${KDToolBox_SOURCE_DIR}/qt/KDSignalThrottler/src/KDSignalThrottler.cpp
                     ${KDToolBox_SOURCE_DIR}/qt/KDSignalThrottler/src/KDSignalThrottler.h
  )

  target_include_directories(kdtoolbox PUBLIC ${KDToolBox_SOURCE_DIR}/qt/KDSignalThrottler/src)
  target_link_libraries(kdtoolbox Qt${QT_VERSION_MAJOR}::Core)
  set_target_properties(kdtoolbox PROPERTIES AUTOMOC ON)
endif()

#TODO: maybe try with e-dant/watcher?
cpmaddpackage(
  NAME
  efsw
  GITHUB_REPOSITORY
  SpartanJ/efsw
  GIT_TAG
  1.7.0
  EXCLUDE_FROM_ALL
  YES
  OPTIONS
  "BUILD_TEST_APP OFF"
)
if(efsw_ADDED)
  target_compile_definitions(efsw PRIVATE EFSW_FSEVENTS_NOT_SUPPORTED)
endif()

if(WIN32)
  set(BUILD_SHARED_LIBS ON)
endif()
cpmaddpackage(
  NAME
  tbb
  GITHUB_REPOSITORY
  uxlfoundation/oneTBB
  VERSION
  2023.1.0
  EXCLUDE_FROM_ALL
  YES
  OPTIONS
  "TBB_TEST OFF"
  "TBB_EXAMPLES OFF"
  "TBB_STRICT OFF"
)

if(WIN32)
  set(BUILD_SHARED_LIBS OFF)
endif()

set(CPM_USE_LOCAL_PACKAGES OFF)
cpmaddpackage(
  NAME
  mimalloc
  GITHUB_REPOSITORY
  microsoft/mimalloc
  VERSION
  3.5.0
  EXCLUDE_FROM_ALL
  YES
  OPTIONS
  "MI_BUILD_TESTS OFF"
  "MI_SECURE OFF"
  "MI_BUILD_SHARED OFF"
  "MI_BUILD_STATIC ON"
  "MI_BUILD_OBJECT OFF"
  "MI_OVERRIDE OFF"
)
if(mimalloc_ADDED)
  message("Adding alias for mimalloc")
  add_library(klogg_mimalloc_wrapper INTERFACE)
  target_link_libraries(klogg_mimalloc_wrapper INTERFACE mimalloc-static)
  target_include_directories(klogg_mimalloc_wrapper INTERFACE ${mimalloc_SOURCE_DIR}/include)
else()
  add_library(klogg_mimalloc_wrapper INTERFACE)
  target_link_libraries(klogg_mimalloc_wrapper INTERFACE ${MIMALLOC_LIBRARY})
  target_include_directories(klogg_mimalloc_wrapper INTERFACE ${MIMALLOC_INCLUDE_DIR})
endif()

set(CPM_USE_LOCAL_PACKAGES ${_TMP_CPM_USE_LOCAL_PACKAGES})

if(KLOGG_USE_SENTRY)
  set(SENTRY_BACKEND
     "crashpad"
     CACHE INTERNAL "" FORCE
  )
  set(SENTRY_TRANSPORT
     "none"
     CACHE INTERNAL "" FORCE
  )
  set(SENTRY_BUILD_EXAMPLES
     OFF
     CACHE INTERNAL "" FORCE
  )
  set(CRASHPAD_ENABLE_INSTALL
     OFF
     CACHE INTERNAL "" FORCE
  )
  set(CRASHPAD_ENABLE_INSTALL_DEV
     OFF
     CACHE INTERNAL "" FORCE
  )

  cpmaddpackage(
    NAME
    sentry
    GITHUB_REPOSITORY
    getsentry/sentry-native
    GIT_TAG
    0.16.4
    EXCLUDE_FROM_ALL
    YES
  )

  if(WIN32)
    set_target_properties(crashpad_handler PROPERTIES OUTPUT_NAME "klogg_crashpad_handler.exe")
  else()
    set_target_properties(crashpad_handler PROPERTIES OUTPUT_NAME "klogg_crashpad_handler")
  endif()
endif(KLOGG_USE_SENTRY)

set(klogg_cpm_targets
    xxhash
    Catch2
    roaring
    libuchardet
    robin_hood
    whereami
    simdutf
    efsw
    SingleApplication
    hs
    tbb
    exprtk
    mimalloc-static
    sentry
    crashpad_client
    crashpad_compat
    crashpad_util
    mini_chromium
    streamvbyte
    FastPFor
)
foreach(target ${klogg_cpm_targets})
  if(TARGET ${target})
    get_property(
      ${target}_include_dirs
      TARGET ${target}
      PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    )
    #message("Marking ${${target}_include_dirs} as system incldue")
    set_property(TARGET ${target} PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${${target}_include_dirs})
  endif()
endforeach()
