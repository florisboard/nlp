project(ICU4C)

include(ProcessorCount)

# Locate GNU make utlility, as ICU requires GNU make.
#  See: https://unicode-org.github.io/icu/userguide/icu4c/build.html#how-to-build-and-install-on-unix
find_program(GNU_MAKE_PROGRAM gmake REQUIRED)

# ----- SETUP LIBRARIES AND FLAGS ----- #

add_library(icu4c INTERFACE)

# We use a custom namespace to avoid any ambiguity with the default ICU namespace
set(ICU_LIBS uc tu i18n io data)
foreach(lib ${ICU_LIBS})
    add_library(icu${lib} STATIC IMPORTED GLOBAL)
    add_library(FlorisICU::${lib} ALIAS icu${lib})
    add_dependencies(icu${lib} icu4c)
endforeach()

# TODO: Reevaluate and fine-tune ICU config and data filter json!!
set(ICU_CONFIGURE_ARGS
    --disable-debug --enable-release --enable-strict --disable-shared --enable-static --disable-auto-cleanup
    --disable-draft --disable-renaming --disable-tracing --disable-plugins --disable-extras --enable-icuio
    --disable-layoutex --enable-tools --disable-fuzzer --disable-tests --disable-samples
    --with-data-packaging=archive --with-library-bits=64 --with-library-suffix=)
set(ICU_C_FLAGS -std=c11 -Os -fPIC -fno-short-wchar -fno-short-enums -fvisibility=hidden)
set(ICU_CXX_FLAGS -std=c++11 -Os -fPIC -fno-short-wchar -fno-short-enums -fvisibility=hidden)
set(ICU_CPP_FLAGS
    -DU_USING_ICU_NAMESPACE=0 -DU_DISABLE_RENAMING=1 -DU_HAVE_NL_LANGINFO_CODESET=0 -DU_HIDE_OBSOLETE_UTF_OLD_H=1
    -DU_TIMEZONE=0 -DUCONFIG_NO_COLLATION=0 -DUCONFIG_NO_FORMATTING=0 -DUCONFIG_NO_LEGACY_CONVERSION=0
    -DUCONFIG_NO_REGULAR_EXPRESSIONS=0 -DUCONFIG_NO_TRANSLITERATION=0)
set(ICU_LDFLAGS -pthread)
set(ICU_DATA_FILTER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/data-feature-filter.json)
set(ICU_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/icu/icu4c)
set(ICU_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
ProcessorCount(NUM_JOBS)

# ----- HOST ----- #

set(ICU_HOST_BUILD_DIR ${ICU_BUILD_DIR}/.intermediate/host)
set(ICU_HOST_INSTALL_DIR ${ICU_BUILD_DIR}/host)
file(MAKE_DIRECTORY ${ICU_HOST_BUILD_DIR}) # Need to make build directory as ICU script requires this
set(ICU_HOST_ENV_CMAKE
    ${CMAKE_COMMAND} -E env ICU_SOURCES="${ICU_SRC_DIR}" ICU_DATA_FILTER_FILE="${ICU_DATA_FILTER_FILE}"
        CC="${CMAKE_C_COMPILER}" CXX="${CMAKE_CXX_COMPILER}" CFLAGS="${ICU_C_FLAGS}" CXXFLAGS="${ICU_CXX_FLAGS}"
        CPPFLAGS="${ICU_CPP_FLAGS}" LDFLAGS="${ICU_LDFLAGS}")
set(ICU_HOST_INSTALL_BYPRODUCTS)
foreach(lib ${ICU_LIBS})
    list(APPEND ICU_HOST_INSTALL_BYPRODUCTS
        ${ICU_HOST_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}icu${lib}${CMAKE_STATIC_LIBRARY_SUFFIX})
endforeach()

add_custom_target(
    icu4c_host_configure
    COMMAND ${ICU_HOST_ENV_CMAKE} ${ICU_SRC_DIR}/source/configure --prefix=${ICU_HOST_INSTALL_DIR} ${ICU_CONFIGURE_ARGS}
    WORKING_DIRECTORY ${ICU_HOST_BUILD_DIR})
add_custom_target(
    icu4c_host
    ALL
    COMMAND ${ICU_HOST_ENV_CMAKE} ${GNU_MAKE_PROGRAM} -j ${NUM_JOBS}
    DEPENDS icu4c_host_configure
    WORKING_DIRECTORY ${ICU_HOST_BUILD_DIR})
add_custom_command(
    TARGET icu4c_host POST_BUILD
    COMMAND ${ICU_HOST_ENV_CMAKE} ${GNU_MAKE_PROGRAM} install
    BYPRODUCTS ${ICU_HOST_INSTALL_BYPRODUCTS}
    WORKING_DIRECTORY ${ICU_HOST_BUILD_DIR})
add_dependencies(icu4c icu4c_host)

set(ICU_LIB_DIR ${ICU_HOST_INSTALL_DIR}/lib)
set(ICU_INCLUDE_DIR ${ICU_HOST_INSTALL_DIR}/include)

# ----- CROSS-COMPILE TO ANDROID ----- #

if(ANDROID)
    message(FATAL_ERROR "TODO: Introduce Android cross-compiling support")
endif()

# ----- FINALIZE PROPERTIES ----- #

# Workaround so CMake stops complaining about non-existent include dir at configure time
file(MAKE_DIRECTORY ${ICU_INCLUDE_DIR})

foreach(lib ${ICU_LIBS})
    target_compile_definitions(icu${lib} INTERFACE ${ICU_CPP_FLAGS})
    set_target_properties(icu${lib} PROPERTIES
        IMPORTED_LOCATION ${ICU_LIB_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}icu${lib}${CMAKE_STATIC_LIBRARY_SUFFIX}
        INTERFACE_INCLUDE_DIRECTORIES ${ICU_INCLUDE_DIR})
endforeach()
