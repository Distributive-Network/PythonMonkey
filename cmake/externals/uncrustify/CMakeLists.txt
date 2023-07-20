# Copyright (c) 2022 Distributive Inc. All Rights Reserved.

set(UNCRUSTIFY_ROOT "${CMAKE_CURRENT_BINARY_DIR}/install" CACHE PATH
  "The Uncrustify root directory."
)
if(NOT UNCRUSTIFY_ROOT STREQUAL "${CMAKE_CURRENT_BINARY_DIR}/install")
  return()
endif()

# 0.1.12
if(CMAKE_SYSTEM_NAME MATCHES "^Darwin$")
  set(UNCRUSTIFY_DOWNLOAD_PACKAGE_FILE_ID "47722390")
  set(UNCRUSTIFY_DOWNLOAD_SHA256
    "f53c51c30f8482cf801bf4db11becc7ec62a7e86f0fc90878150e9a85bb7638b"
  )
elseif(CMAKE_SYSTEM_NAME MATCHES "^Linux$")
  set(UNCRUSTIFY_DOWNLOAD_PACKAGE_FILE_ID "47722428")
  set(UNCRUSTIFY_DOWNLOAD_SHA256
    "595a4634831777bf77612ca0223ef5cf8da9aac4aee885291a886bf9b358153d"
  )
elseif(CMAKE_SYSTEM_NAME MATCHES "^Windows$")
  set(UNCRUSTIFY_DOWNLOAD_PACKAGE_FILE_ID "47722782")
  set(UNCRUSTIFY_DOWNLOAD_SHA256
    "2505c9397c0a6f96c16570044bd21ea7992f27118cc1f2e3d7a6945b8e2a4702"
  )
else()
  message(WARNING "No prebuilt Uncrustify library for this platform.")
  return()
endif()

string(CONCAT UNCRUSTIFY_DOWNLOAD_URL
  "https://gitlab.com/Distributed-Compute-Protocol/uncrustify-build/-/"
  "package_files/${UNCRUSTIFY_DOWNLOAD_PACKAGE_FILE_ID}/download"
)

include("${CMAKE_ROOT}/Modules/FetchContent.cmake")

FetchContent_Declare(Uncrustify
  DOWNLOAD_EXTRACT_TIMESTAMP FALSE
  PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
  SOURCE_DIR "${UNCRUSTIFY_ROOT}"
  URL "${UNCRUSTIFY_DOWNLOAD_URL}"
  URL_HASH "SHA256=${UNCRUSTIFY_DOWNLOAD_SHA256}"
  TLS_VERIFY TRUE
)
FetchContent_MakeAvailable(Uncrustify)
