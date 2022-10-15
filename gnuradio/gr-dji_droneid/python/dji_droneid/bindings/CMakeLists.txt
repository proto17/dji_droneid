# Copyright 2020 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Check if there is C++ code at all
########################################################################
if(NOT dji_droneid_sources)
    MESSAGE(STATUS "No C++ sources... skipping python bindings")
    return()
endif(NOT dji_droneid_sources)

########################################################################
# Check for pygccxml
########################################################################
GR_PYTHON_CHECK_MODULE_RAW(
    "pygccxml"
    "import pygccxml"
    PYGCCXML_FOUND
    )

include(GrPybind)

########################################################################
# Python Bindings
########################################################################

list(APPEND dji_droneid_python_files
    utils_python.cc
    burst_extractor_python.cc
    detector_python.cc python_bindings.cc)

GR_PYBIND_MAKE_OOT(dji_droneid
   ../../..
   gr::dji_droneid
   "${dji_droneid_python_files}")

# copy bindings extension for use in QA test module
add_custom_command(TARGET dji_droneid_python POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:dji_droneid_python>
    ${PROJECT_BINARY_DIR}/test_modules/gnuradio/dji_droneid/
)

install(TARGETS dji_droneid_python DESTINATION ${GR_PYTHON_DIR}/gnuradio/dji_droneid COMPONENT pythonapi)
