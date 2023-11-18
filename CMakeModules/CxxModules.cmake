function(target_module_sources target_name target_type mod_src1)
    target_sources(${target_name} ${target_type} FILE_SET cxx_modules TYPE CXX_MODULES FILES ${mod_src1} ${ARGN})
endfunction()
