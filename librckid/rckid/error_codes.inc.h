#ifndef ERROR_CODE
#error "ERROR_CODE macro must be provided before including error_codes.inc.h"
#endif

ERROR_CODE(None, "No error")
ERROR_CODE(Internal, "Internal Error")
ERROR_CODE(AssertFailure, "Assertion Failure")
ERROR_CODE(OutOfMemory, "Heap out of memor error")

// debug only errors that should not be firing in production
ERROR_CODE(Unimplemented, "Unimplemented error")
ERROR_CODE(Unreachable, "Unreachable error")

#undef ERROR_CODE