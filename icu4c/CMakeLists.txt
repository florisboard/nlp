project(ICU4C)

include(ProcessorCount)

# Locate GNU make utlility, as ICU requires GNU make.
#  See: https://unicode-org.github.io/icu/userguide/icu4c/build.html#how-to-build-and-install-on-unix
find_program(GNU_MAKE_PROGRAM gmake REQUIRED)

# ----- SETUP LIBRARIES, HELPER FUNCTIONS AND FLAGS ----- #

set(ICU_VERSION_MAJOR 73)
set(ICU_VERSION_MINOR 1)
# TODO: can we find this dynamically??
set(ICU_HOST_TARGET "x86_64-unknown-linux-gnu")

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
    --with-data-packaging=archive --with-library-bits=nochange --with-library-suffix=${ICU_LIB_NAME_SUFFIX})
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
set(ICU_HOST_C_FLAGS -target ${ICU_HOST_TARGET} ${ICU_C_FLAGS})
set(ICU_HOST_CXX_FLAGS -target ${ICU_HOST_TARGET} ${ICU_CXX_FLAGS})
set(ICU_HOST_ENV_CMAKE
    ${CMAKE_COMMAND} -E env ICU_SOURCES="${ICU_SRC_DIR}" ICU_DATA_FILTER_FILE="${ICU_DATA_FILTER_FILE}"
        CC="${CMAKE_C_COMPILER}" CXX="${CMAKE_CXX_COMPILER}" CFLAGS="${ICU_HOST_C_FLAGS}"
        CXXFLAGS="${ICU_HOST_CXX_FLAGS}" CPPFLAGS="${ICU_CPP_FLAGS}" LDFLAGS="${ICU_LDFLAGS}")
set(ICU_HOST_INSTALL_BYPRODUCTS)
foreach(lib ${ICU_LIBS})
    if(NOT ANDROID)
        list(APPEND ICU_HOST_INSTALL_BYPRODUCTS
            ${ICU_HOST_INSTALL_DIR}/lib/${ICU_LIB_PREFIX}icu${lib}${ICU_LIB_SUFFIX}
            ${FLORIS_LIBRARY_OUTPUT_DIRECTORY}/${ICU_LIB_PREFIX}icu${lib}${ICU_LIB_SUFFIX})
    else()
        list(APPEND ICU_HOST_INSTALL_BYPRODUCTS
        ${ICU_HOST_INSTALL_DIR}/lib/${ICU_LIB_PREFIX}icu${lib}${ICU_LIB_SUFFIX})
    endif()
endforeach()

add_custom_target(
    icu4c_host_configure
    COMMAND ${ICU_HOST_ENV_CMAKE} ${ICU_SRC_DIR}/source/configure
      -q --prefix=${ICU_HOST_INSTALL_DIR} ${ICU_CONFIGURE_ARGS}
    WORKING_DIRECTORY ${ICU_HOST_BUILD_DIR})
add_custom_target(
    icu4c_host_build
    COMMAND ${ICU_HOST_ENV_CMAKE} ${GNU_MAKE_PROGRAM} -j ${NUM_JOBS} >/dev/null
    DEPENDS icu4c_host_configure
    WORKING_DIRECTORY ${ICU_HOST_BUILD_DIR})
add_custom_target(
    icu4c_host
    COMMAND ${ICU_HOST_ENV_CMAKE} ${GNU_MAKE_PROGRAM} install >/dev/null
    DEPENDS icu4c_host_build
    BYPRODUCTS ${ICU_HOST_INSTALL_BYPRODUCTS}
    WORKING_DIRECTORY ${ICU_HOST_BUILD_DIR})
add_dependencies(icu4c icu4c_host)

set(ICU_LIB_DIR ${ICU_HOST_INSTALL_DIR}/lib)
set(ICU_INCLUDE_DIR ${ICU_HOST_INSTALL_DIR}/include)
set(ICU_TARGET icu4c_host)

# ----- CROSS-COMPILE TO ANDROID ----- #

if(ANDROID)
    if(CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
        set(ICU_ANDROID_TARGET "armv7a-linux-androideabi")
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
        set(ICU_ANDROID_TARGET "aarch64-linux-android")
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86")
        set(ICU_ANDROID_TARGET "i686-linux-android")
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
        set(ICU_ANDROID_TARGET "x86_64-linux-android")
    else()
        message(FATAL_ERROR "Unsupported ABI: ${CMAKE_ANDROID_ARCH_ABI}")
    endif()

    set(ICU_ANDROID_BUILD_DIR ${ICU_BUILD_DIR}/.intermediate/android/${CMAKE_ANDROID_ARCH_ABI})
    set(ICU_ANDROID_INSTALL_DIR ${ICU_BUILD_DIR}/android/${CMAKE_ANDROID_ARCH_ABI})
    file(MAKE_DIRECTORY ${ICU_ANDROID_BUILD_DIR}) # Need to make build directory as ICU script requires this
    get_filename_component(TOOLCHAIN ${CMAKE_C_COMPILER} DIRECTORY)
    get_filename_component(TOOLCHAIN ${TOOLCHAIN} DIRECTORY)
    set(ICU_ANDROID_CC ${TOOLCHAIN}/bin/${ICU_ANDROID_TARGET}${CMAKE_ANDROID_API}-clang)
    set(ICU_ANDROID_CXX ${TOOLCHAIN}/bin/${ICU_ANDROID_TARGET}${CMAKE_ANDROID_API}-clang++)
    set(ICU_ANDROID_LDFLAGS -lc -lstdc++ -Wl,--gc-sections,-rpath-link=${TOOLCHAIN}/sysroot/usr/lib/)
    set(ICU_ANDROID_ENV_CMAKE
        ${CMAKE_COMMAND} -E env
        TOOLCHAIN=${TOOLCHAIN}
        TARGET=${ICU_ANDROID_TARGET}
        NDK_STANDARD_ROOT=${TOOLCHAIN}
        API=${CMAKE_ANDROID_API}
        ANDROIDVER=${CMAKE_ANDROID_API}
        AR=${TOOLCHAIN}/bin/llvm-ar
        AS=${ICU_ANDROID_CC}
        CC=${ICU_ANDROID_CC}
        CXX=${ICU_ANDROID_CXX}
        LD=${TOOLCHAIN}/bin/ld
        RANLIB=${TOOLCHAIN}/bin/llvm-ranlib
        STRIP=${TOOLCHAIN}/bin/llvm-strip
        CFLAGS="${ICU_C_FLAGS}"
        CXXFLAGS="${ICU_CXX_FLAGS}"
        CPPFLAGS="${ICU_CPP_FLAGS}"
        LDFLAGS="${ICU_ANDROID_LDFLAGS}"
        ICU_SOURCES=${ICU_SRC_DIR}
        ICU_DATA_FILTER_FILE=${ICU_DATA_FILTER_FILE}
        ICU_CROSS_BUILD=${ICU_HOST_BUILD_DIR}
    )
    set(ICU_ANDROID_INSTALL_BYPRODUCTS)
    foreach(lib ${ICU_LIBS})
        list(APPEND ICU_ANDROID_INSTALL_BYPRODUCTS
            ${ICU_ANDROID_INSTALL_DIR}/lib/${ICU_LIB_PREFIX}icu${lib}${ICU_LIB_SUFFIX}
            ${FLORIS_LIBRARY_OUTPUT_DIRECTORY}/${ICU_LIB_PREFIX}icu${lib}${ICU_LIB_SUFFIX})
    endforeach()

    add_custom_target(
        icu4c_android_configure
        COMMAND ${ICU_ANDROID_ENV_CMAKE} ${ICU_SRC_DIR}/source/configure --with-cross-build=${ICU_HOST_BUILD_DIR}
            --host=${ICU_ANDROID_TARGET} --prefix=${ICU_ANDROID_INSTALL_DIR} ${ICU_CONFIGURE_ARGS}
        DEPENDS icu4c_host
        WORKING_DIRECTORY ${ICU_ANDROID_BUILD_DIR})
    add_custom_target(
        icu4c_android_build
        COMMAND ${ICU_ANDROID_ENV_CMAKE} ${GNU_MAKE_PROGRAM} -j ${NUM_JOBS}
        DEPENDS icu4c_android_configure
        WORKING_DIRECTORY ${ICU_ANDROID_BUILD_DIR})
    add_custom_target(
        icu4c_android
        COMMAND ${ICU_ANDROID_ENV_CMAKE} ${GNU_MAKE_PROGRAM} install
        DEPENDS icu4c_android_build
        BYPRODUCTS ${ICU_ANDROID_INSTALL_BYPRODUCTS}
        WORKING_DIRECTORY ${ICU_ANDROID_BUILD_DIR})
    add_dependencies(icu4c icu4c_android)

    set(ICU_LIB_DIR ${ICU_ANDROID_INSTALL_DIR}/lib)
    set(ICU_INCLUDE_DIR ${ICU_ANDROID_INSTALL_DIR}/include)
    set(ICU_TARGET icu4c_android)
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

if(ICU_ASSET_EXPORT_DIR)
    set(src_asset_dir ${ICU_HOST_INSTALL_DIR}/share/icu${ICU_LIB_NAME_SUFFIX}/${ICU_VERSION_MAJOR}.${ICU_VERSION_MINOR})
    set(dst_asset_dir ${ICU_ASSET_EXPORT_DIR})
    add_custom_command(TARGET ${ICU_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${src_asset_dir}/icudt${ICU_VERSION_MAJOR}l.dat
            ${dst_asset_dir}/icudt.dat)
    add_custom_command(TARGET ${ICU_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${src_asset_dir}/LICENSE
            ${dst_asset_dir}/LICENSE)
endif()

# Add function and cache for compiler definitions
# This is vital or any target which links against any ICU lib will not compile!!
set_property(GLOBAL PROPERTY __icu_cpp_flags ${ICU_CPP_FLAGS})
function(target_link_icu4c_libraries target_name target_type lib1)
    get_property(icu_cpp_flags GLOBAL PROPERTY __icu_cpp_flags)
    target_compile_definitions(${target_name} ${target_type} ${icu_cpp_flags})
    target_link_libraries(${target_name} ${target_type} ${lib1} ${ARGN})
endfunction()
