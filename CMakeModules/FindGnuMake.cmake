find_program(GNU_MAKE_PROGRAM gmake)
if (NOT GNU_MAKE_PROGRAM)
    find_program(GNU_MAKE_PROGRAM make)
    if (NOT GNU_MAKE_PROGRAM)
        message(FATAL_ERROR "Failed to locate any make utility on this system.")
    endif()
endif()

execute_process(COMMAND ${GNU_MAKE_PROGRAM} --version OUTPUT_VARIABLE GNU_MAKE_VERSION_OUTPUT ERROR_QUIET)
string(REGEX MATCH "GNU Make" IS_GNU_MAKE "${GNU_MAKE_VERSION_OUTPUT}")

if (NOT IS_GNU_MAKE)
    message(FATAL_ERROR "Failed to locate GNU make on this system.")
endif()
