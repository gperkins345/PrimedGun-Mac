set(MZ_COMPAT OFF)
set(MZ_BUILD_TESTS OFF)
set(MZ_BUILD_UNIT_TESTS OFF)
set(MZ_BUILD_FUZZ_TESTS OFF)
set(MZ_CODE_COVERAGE OFF)
set(MZ_FETCH_LIBS OFF CACHE BOOL "Disable minizip-ng vendored dependency fetch/build." FORCE)
set(MZ_FORCE_FETCH_LIBS OFF CACHE BOOL "Disable minizip-ng forced vendored dependency fetch/build." FORCE)
# QuestPrimeVR: on macOS, use Apple's native CommonCrypto instead of OpenSSL (libcrypto is not
# available/linked on this Mac), otherwise the final link fails with undefined EVP_/AES_/SHA_ symbols.
if(APPLE)
  set(MZ_OPENSSL OFF CACHE BOOL "Use CommonCrypto instead of OpenSSL on macOS." FORCE)
endif()
set(SKIP_INSTALL_ALL ON)

add_subdirectory(minizip-ng)
dolphin_disable_warnings(minizip-ng)
