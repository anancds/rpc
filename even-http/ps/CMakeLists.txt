
add_subdirectory(core)
set(HTTP_SRC
        util.cc
        )
set(HTTP_HEADER
        util.h
        )

foreach (_target
        util_test
        )
    add_executable(${_target} "${_target}.cpp"
            ${HTTP_SRC}
            ${HTTP_HEADER})
    target_link_libraries(${_target}
            rpc-log
            )
endforeach ()