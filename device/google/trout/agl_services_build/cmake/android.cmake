if(NOT trout_ANDROID_SYSCORE_DIR)
    set(trout_ANDROID_SYSCORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/android/system_core")
endif()

set(_trout_ANDROID_CXX_FLAGS -Wall -Werror -Wextra -Wno-unknown-warning-option -Wno-c99-designator -std=c++17)

set(trout_ANDROID_LIBLOG_DIR ${trout_ANDROID_SYSCORE_DIR}/liblog)
set(trout_ANDROID_LIBLOG_INCLUDE_DIR ${trout_ANDROID_LIBLOG_DIR}/include)
set(trout_ANDROID_LIBLOG_LIBRARY "android_liblog")

set(trout_ANDROID_LIBBASE_DIR ${trout_ANDROID_SYSCORE_DIR}/base)
set(trout_ANDROID_LIBBASE_INCLUDE_DIR ${trout_ANDROID_LIBBASE_DIR}/include)
set(trout_ANDROID_LIBBASE_LIBRARY "android_libbase")

set(trout_ANDROID_LIBUTILS_DIR ${trout_ANDROID_SYSCORE_DIR}/libutils)
set(trout_ANDROID_LIBUTLS_INCLUDE_DIR ${trout_ANDROID_LIBUTILS_DIR}/include)
set(trout_ANDROID_LIBUTILS_LIBRARY "android_libutils")

set(trout_ANDROID_LIBCUTLS_INCLUDE_DIR ${trout_ANDROID_SYSCORE_DIR}/libcutils/include)


# =========== libbase =================

if (NOT TARGET ${trout_ANDROID_LIBBASE_LIBRARY})
    add_library(${trout_ANDROID_LIBBASE_LIBRARY}
        ${trout_ANDROID_LIBBASE_DIR}/liblog_symbols.cpp
        ${trout_ANDROID_LIBBASE_DIR}/logging.cpp
        ${trout_ANDROID_LIBBASE_DIR}/strings.cpp
        ${trout_ANDROID_LIBBASE_DIR}/stringprintf.cpp
        ${trout_ANDROID_LIBBASE_DIR}/threads.cpp
    )

    target_include_directories(${trout_ANDROID_LIBBASE_LIBRARY}
        PUBLIC ${trout_ANDROID_LIBBASE_INCLUDE_DIR}
        PRIVATE ${trout_FMTLIB_INCLUDE_DIRS}
    )

    target_link_libraries(${trout_ANDROID_LIBBASE_LIBRARY}
        ${trout_ANDROID_LIBLOG_LIBRARY}
        ${trout_FMTLIB_LIBRARIES}
    )

    target_compile_options(${trout_ANDROID_LIBBASE_LIBRARY} PRIVATE ${_trout_ANDROID_CXX_FLAGS})
endif()


# =========== liblog =================

if (NOT TARGET ${trout_ANDROID_LIBLOG_LIBRARY})
    add_library(${trout_ANDROID_LIBLOG_LIBRARY}
        ${trout_ANDROID_LIBLOG_DIR}/logger_write.cpp
        ${trout_ANDROID_LIBLOG_DIR}/properties.cpp
    )

    target_include_directories(${trout_ANDROID_LIBLOG_LIBRARY}
        PUBLIC ${trout_ANDROID_LIBLOG_INCLUDE_DIR}
        PRIVATE ${trout_ANDROID_LIBBASE_INCLUDE_DIR}
        PRIVATE ${trout_ANDROID_LIBCUTLS_INCLUDE_DIR}
    )

    target_compile_options(${trout_ANDROID_LIBLOG_LIBRARY} PRIVATE ${_trout_ANDROID_CXX_FLAGS})
endif()


# =========== libutils =================

if (NOT TARGET ${trout_ANDROID_LIBUTILS_LIBRARY})
    add_library(${trout_ANDROID_LIBUTILS_LIBRARY}
         ${trout_ANDROID_LIBUTILS_DIR}/SystemClock.cpp
         ${trout_ANDROID_LIBUTILS_DIR}/Timers.cpp
    )

    target_include_directories(${trout_ANDROID_LIBUTILS_LIBRARY}
        PUBLIC ${trout_ANDROID_LIBUTLS_INCLUDE_DIR}
        PRIVATE ${trout_ANDROID_LIBCUTLS_INCLUDE_DIR}
    )

    target_link_libraries(${trout_ANDROID_LIBUTILS_LIBRARY}
        ${trout_ANDROID_LIBLOG_LIBRARY}
    )

    target_compile_options(${trout_ANDROID_LIBUTILS_LIBRARY} PRIVATE ${_trout_ANDROID_CXX_FLAGS})
endif()


# =========== export libraries =================

set(trout_ANDROID_INCLUDE_DIRS
    ${trout_ANDROID_LIBBASE_INCLUDE_DIR}
    ${trout_ANDROID_LIBCUTLS_INCLUDE_DIR}
)

set(trout_ANDROID_LIBRARIES
	${trout_ANDROID_LIBBASE_LIBRARY}
	${trout_ANDROID_LIBLOG_LIBRARY}
	${trout_ANDROID_LIBUTILS_LIBRARY}
)
