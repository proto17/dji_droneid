find_package(PkgConfig)

PKG_CHECK_MODULES(PC_GR_DRONEID gnuradio-droneid)

FIND_PATH(
    GR_DRONEID_INCLUDE_DIRS
    NAMES gnuradio/droneid/api.h
    HINTS $ENV{DRONEID_DIR}/include
        ${PC_DRONEID_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    GR_DRONEID_LIBRARIES
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

include("${CMAKE_CURRENT_LIST_DIR}/gnuradio-droneidTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GR_DRONEID DEFAULT_MSG GR_DRONEID_LIBRARIES GR_DRONEID_INCLUDE_DIRS)
MARK_AS_ADVANCED(GR_DRONEID_LIBRARIES GR_DRONEID_INCLUDE_DIRS)
