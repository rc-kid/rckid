cmake_minimum_required(VERSION 3.12)

# SDK tests
#
# The unittests are only available when building for the FANTASY console backend. 
# TODO does it have to be? 
if (RCKID_BACKEND STREQUAL "FANTASY")

    add_executable(dolangc "dolangc.cpp")
    link_with_librckid(dolangc)

else()
    message("Tests are only available in RCKID_BACKEND=FANTASY, skipping")
endif()
