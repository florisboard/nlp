project(ICU4C)

include(ProcessorCount)

# Locate GNU make utlility, as ICU requires GNU make.
#  See: https://unicode-org.github.io/icu/userguide/icu4c/build.html#how-to-build-and-install-on-unix
find_program(GNU_MAKE_PROGRAM gmake REQUIRED)

# ----- SETUP LIBRARIES AND FLAGS ----- #
# We use a custom namespace to avoid any ambiguity with the default ICU namespace
set(ICU_LIBS uc tu i18n io data)
foreach(lib ${ICU_LIBS})
    add_library(icu${lib} STATIC IMPORTED GLOBAL)
    add_library(FlorisICU::${lib} ALIAS icu${lib})
endforeach()

set(ICU_CONFIGURE_ARGS
    --disable-debug --enable-release --enable-strict --disable-shared --enable-static --disable-auto-cleanup
    --disable-draft --disable-renaming --disable-tracing --disable-plugins --disable-extras --enable-icuio
    --disable-layoutex --enable-tools --disable-fuzzer --disable-tests --disable-samples
    --with-data-packaging=archive --with-library-bits=64 --with-library-suffix=)
set(ICU_FLAGS
    -fPIC -fno-short-wchar -fno-short-enums -fvisibility=hidden -DU_USING_ICU_NAMESPACE=0 -DU_DISABLE_RENAMING=1
    -DU_HAVE_NL_LANGINFO_CODESET=0 -DU_HIDE_OBSOLETE_UTF_OLD_H=1 -DU_TIMEZONE=0 -DUCONFIG_NO_COLLATION=0
    -DUCONFIG_NO_FORMATTING=0 -DUCONFIG_NO_LEGACY_CONVERSION=0 -DUCONFIG_NO_REGULAR_EXPRESSIONS=0
    -DUCONFIG_NO_TRANSLITERATION=0)
set(ICU_C_FLAGS -std=c11 ${ICU_FLAGS})
set(ICU_CXX_FLAGS -std=c++11 ${ICU_FLAGS})
set(ICU_LDFLAGS -pthread)
set(ICU_DATA_FILTER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/data-feature-filter.json)
set(ICU_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/icu/icu4c)
set(ICU_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
ProcessorCount(NUM_JOBS)

# ----- HOST ----- #

set(ICU_HOST_BUILD_DIR ${ICU_BUILD_DIR}/host)
set(ICU_HOST_INSTALL_DIR ${ICU_HOST_BUILD_DIR}/icu_build)
set(ICU_HOST_ENV_CMAKE
    ${CMAKE_COMMAND} -E env ICU_SOURCES="${ICU_SRC_DIR}" ICU_DATA_FILTER_FILE="${ICU_DATA_FILTER_FILE}"
        CC="${CMAKE_C_COMPILER}" CXX="${CMAKE_CXX_COMPILER}" CFLAGS="${ICU_C_FLAGS}" CXXFLAGS="${ICU_CXX_FLAGS}"
        LDFLAGS="${ICU_LDFLAGS}")
file(MAKE_DIRECTORY ${ICU_HOST_BUILD_DIR})
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

foreach(lib ${ICU_LIBS})
    add_dependencies(icu${lib} icu4c_host)
    if(NOT ANDROID)
        set_target_properties(icu${lib} PROPERTIES IMPORTED_LOCATION ${ICU_HOST_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}icu${lib}${CMAKE_STATIC_LIBRARY_SUFFIX})
        target_include_directories(icu${lib} INTERFACE $<INSTALL_INTERFACE:${ICU_HOST_INSTALL_DIR}/include>)
    endif()
endforeach()
#${ICU_HOST_BUILD_DIR}/data/out/icudt71l.dat
