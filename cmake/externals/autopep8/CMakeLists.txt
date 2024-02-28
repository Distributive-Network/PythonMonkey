# Copyright (c) 2022 Distributive Inc. All Rights Reserved.

set(AUTOPEP8_ROOT "${CMAKE_CURRENT_BINARY_DIR}/install" CACHE PATH
  "The autopep8 root directory."
)
if(NOT AUTOPEP8_ROOT STREQUAL "${CMAKE_CURRENT_BINARY_DIR}/install")
  return()
endif()

  set(AUTOPEP8_DOWNLOAD_PACKAGE_FILE_ID "5d/9b/1ed75f8c9086fafe0e9bbb379a70c43b1aa9dff6154ddcfb818f78cb0736/autopep8-1.7.0-py2.py3-none-any.whl")
  set(AUTOPEP8_DOWNLOAD_SHA256
    "6f09e90a2be784317e84dc1add17ebfc7abe3924239957a37e5040e27d812087"
  )

string(CONCAT AUTOPEP8_DOWNLOAD_URL
  "https://files.pythonhosted.org/"
  "packages/${AUTOPEP8_DOWNLOAD_PACKAGE_FILE_ID}"
)

include("${CMAKE_ROOT}/Modules/FetchContent.cmake")

FetchContent_Declare(autopep8
  DOWNLOAD_EXTRACT_TIMESTAMP FALSE
  PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
  SOURCE_DIR "${AUTOPEP8_ROOT}"
  URL "${AUTOPEP8_DOWNLOAD_URL}"
  URL_HASH "SHA256=${AUTOPEP8_DOWNLOAD_SHA256}"
  TLS_VERIFY TRUE
)
FetchContent_MakeAvailable(autopep8)
