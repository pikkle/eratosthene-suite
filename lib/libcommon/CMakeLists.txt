cmake_minimum_required(VERSION 3.15)
project(libcommon)

set(CMAKE_CXX_STANDARD 17)

set(HEADERS
        src/common.h
        src/common-args.h
        src/common-image.h
        src/common-include.h
        src/common-time.h
        )

set(SOURCES
        src/common-args.c
        src/common-image.c
        src/common-time.c
        )

add_library(common STATIC ${SOURCES})
install(TARGETS common DESTINATION lib)
install(FILES ${HEADERS} DESTINATION include)