INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_DRONEID droneid)

FIND_PATH(
    DRONEID_INCLUDE_DIRS
    NAMES droneid/api.h
    HINTS $ENV{DRONEID_DIR}/include
        ${PC_DRONEID_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    DRONEID_LIBRARIES
    NAMES gnuradio-droneid
    HINTS $ENV{DRONEID_DIR}/lib
        ${PC_DRONEID_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include("${CMAKE_CURRENT_LIST_DIR}/droneidTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DRONEID DEFAULT_MSG DRONEID_LIBRARIES DRONEID_INCLUDE_DIRS)
MARK_AS_ADVANCED(DRONEID_LIBRARIES DRONEID_INCLUDE_DIRS)
