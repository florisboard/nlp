project(ICU4C)

include(ProcessorCount)

# Locate GNU make utlility, as ICU requires GNU make.
#  See: https://unicode-org.github.io/icu/userguide/icu4c/build.html#how-to-build-and-install-on-unix
find_program(GNU_MAKE_PROGRAM gmake REQUIRED)

# ----- SETUP LIBRARIES, HELPER FUNCTIONS AND FLAGS ----- #

set(ICU_VERSION_MAJOR 72)
set(ICU_VERSION_MINOR 1)

add_library(icu4c INTERFACE)

# We use a custom namespace to avoid any ambiguity with the default ICU namespace
set(ICU_LIBS uc tu i18n io data)
foreach(lib ${ICU_LIBS})
    add_library(icu${lib} ${FLORIS_LIBRARY_TYPE} IMPORTED GLOBAL)
    add_library(FlorisICU::${lib} ALIAS icu${lib})
    add_dependencies(icu${lib} icu4c)
endforeach()

# TODO: Reevaluate and fine-tune ICU config and data filter json!!
set(ICU_LIB_NAME_SUFFIX _floris)
if(BUILD_SHARED_LIBS)
    set(ICU_LIB_PREFIX ${FLORIS_LIBRARY_PREFIX})
    set(ICU_LIB_SUFFIX ${ICU_LIB_NAME_SUFFIX}${FLORIS_LIBRARY_SUFFIX}.${ICU_VERSION_MAJOR})
    set(configure_args_prefix --enable-shared --disable-static)
    set(cpp_flags_prefix)
else()
    set(ICU_LIB_PREFIX ${FLORIS_LIBRARY_PREFIX})
    set(ICU_LIB_SUFFIX ${ICU_LIB_NAME_SUFFIX}${FLORIS_LIBRARY_SUFFIX})
    set(configure_args_prefix --disable-shared --enable-static)
    set(cpp_flags_prefix -DU_STATIC_IMPLEMENTATION)
endif()
set(ICU_CONFIGURE_ARGS
    ${configure_args_prefix} --disable-debug --enable-release --enable-strict  --disable-auto-cleanup
    --disable-draft --disable-renaming --disable-tracing --disable-plugins --disable-extras --enable-icuio
    --disable-layoutex --enable-tools --disable-fuzzer --disable-tests --disable-samples
    --with-data-packaging=archive --with-library-bits=64 --with-library-suffix=${ICU_LIB_NAME_SUFFIX})
set(ICU_C_FLAGS -std=c11 -Os -fPIC -fno-short-wchar -fno-short-enums -fvisibility=hidden)
set(ICU_CXX_FLAGS -std=c++11 -Os -fPIC -fno-short-wchar -fno-short-enums -fvisibility=hidden)
set(ICU_CPP_FLAGS
    ${cpp_flags_prefix} -DU_USING_ICU_NAMESPACE=0 -DU_DISABLE_RENAMING=1 -DU_HAVE_NL_LANGINFO_CODESET=0
    -DU_HIDE_OBSOLETE_UTF_OLD_H=1 -DU_TIMEZONE=0 -DUCONFIG_NO_COLLATION=0 -DUCONFIG_NO_FORMATTING=0
    -DUCONFIG_NO_LEGACY_CONVERSION=0 -DUCONFIG_NO_REGULAR_EXPRESSIONS=0 -DUCONFIG_NO_TRANSLITERATION=0)
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
        ${ICU_HOST_INSTALL_DIR}/lib/${ICU_LIB_PREFIX}icu${lib}${ICU_LIB_SUFFIX}
        ${FLORIS_LIBRARY_OUTPUT_DIRECTORY}/${ICU_LIB_PREFIX}icu${lib}${ICU_LIB_SUFFIX})
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
set(ICU_TARGET icu4c_host)

# ----- CROSS-COMPILE TO ANDROID ----- #

if(ANDROID)
    message(FATAL_ERROR "TODO: Introduce Android cross-compiling support")
endif()

# ----- FINALIZE ----- #

# Workaround so CMake stops complaining about non-existent include dir at configure time
file(MAKE_DIRECTORY ${ICU_INCLUDE_DIR})

foreach(lib ${ICU_LIBS})
    set(intermediate_lib_path ${ICU_LIB_DIR}/${ICU_LIB_PREFIX}icu${lib}${ICU_LIB_SUFFIX})
    set(lib_path ${FLORIS_LIBRARY_OUTPUT_DIRECTORY}/${ICU_LIB_PREFIX}icu${lib}${ICU_LIB_SUFFIX})
    add_custom_command(TARGET ${ICU_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${intermediate_lib_path} ${lib_path})
    target_compile_definitions(icu${lib} INTERFACE ${ICU_CPP_FLAGS})
    set_target_properties(icu${lib} PROPERTIES
        IMPORTED_LOCATION ${lib_path}
        INTERFACE_INCLUDE_DIRECTORIES ${ICU_INCLUDE_DIR})
endforeach()

# Add function and cache for compiler definitions
# This is vital or any target which links against any ICU lib will not compile!!
set_property(GLOBAL PROPERTY __icu_cpp_flags ${ICU_CPP_FLAGS})
function(target_link_icu4c_libraries target_name target_type lib1)
    get_property(icu_cpp_flags GLOBAL PROPERTY __icu_cpp_flags)
    target_compile_definitions(${target_name} ${target_type} ${icu_cpp_flags})
    target_link_libraries(${target_name} ${target_type} ${lib1} ${ARGN})
endfunction()
