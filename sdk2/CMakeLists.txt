cmake_minimum_required(VERSION 3.12)

# add the backend as selected
string(TOLOWER "${RCKID_BACKEND}" BACKEND_LOWER)
add_subdirectory("backend/${BACKEND_LOWER}")

# TODO add sdk cmakelists

