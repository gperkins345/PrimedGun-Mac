if(NOT DEFINED PRIMEGUN_USER_DEFAULTS_SOURCE)
  message(FATAL_ERROR "PRIMEGUN_USER_DEFAULTS_SOURCE is not set")
endif()

if(NOT DEFINED PRIMEGUN_USER_DEFAULTS_DEST)
  message(FATAL_ERROR "PRIMEGUN_USER_DEFAULTS_DEST is not set")
endif()

if(NOT EXISTS "${PRIMEGUN_USER_DEFAULTS_SOURCE}")
  return()
endif()

file(MAKE_DIRECTORY "${PRIMEGUN_USER_DEFAULTS_DEST}")

file(GLOB_RECURSE primegun_user_default_entries
  LIST_DIRECTORIES true
  RELATIVE "${PRIMEGUN_USER_DEFAULTS_SOURCE}"
  "${PRIMEGUN_USER_DEFAULTS_SOURCE}/*"
)

foreach(relative_path IN LISTS primegun_user_default_entries)
  if(relative_path MATCHES "^(Cache|Dump|Logs|GC|Wii)(/|$)")
    continue()
  endif()

  if(relative_path MATCHES "(^|/)[^/]*\\.(bak|cache|log|gci|raw)$")
    continue()
  endif()

  if(relative_path MATCHES "(^|/)(SYSCONF|fst\\.bin)$")
    continue()
  endif()

  set(source_path "${PRIMEGUN_USER_DEFAULTS_SOURCE}/${relative_path}")
  set(dest_path "${PRIMEGUN_USER_DEFAULTS_DEST}/${relative_path}")

  if(IS_DIRECTORY "${source_path}")
    file(MAKE_DIRECTORY "${dest_path}")
  else()
    get_filename_component(dest_dir "${dest_path}" DIRECTORY)
    file(MAKE_DIRECTORY "${dest_dir}")

    if(NOT EXISTS "${dest_path}")
      file(COPY_FILE "${source_path}" "${dest_path}")
    endif()
  endif()
endforeach()
