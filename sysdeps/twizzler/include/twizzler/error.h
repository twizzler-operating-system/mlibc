#pragma once

#include<stddef.h>
#include<stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t twz_error;
typedef uint16_t twz_error_code;
typedef uint16_t twz_error_category;

const twz_error ERROR_CODE_SHIFT = 0;
const twz_error ERROR_CODE_MASK = 0x0000FFFF;
const twz_error ERROR_CATEGORY_MASK = 0x0000FFFF00000000;
const twz_error ERROR_CATEGORY_SHIFT = 32;

const twz_error_code SUCCESS = 0;

// Generic
const twz_error_code OTHER_ERROR = 0;
const twz_error_code NOT_SUPPORTED = 1;
const twz_error_code INTERNAL = 2;
const twz_error_code WOULD_BLOCK = 3;
const twz_error_code TIMED_OUT = 4;
const twz_error_code ACCESS_DENIED = 5;
const twz_error_code NO_SUCH_OPERATION = 6;
const twz_error_code INTERRUPTED = 7;
const twz_error_code IN_PROGRESS = 8;

// Argument
const twz_error_code INVALID_ARGUMENT = 1;
const twz_error_code WRONG_TYPE = 2;
const twz_error_code INVALID_ADDRESS = 3;
const twz_error_code BAD_HANDLE = 4;

// Resource
const twz_error_code OUT_OF_MEMORY = 1;
const twz_error_code OUT_OF_RESOURCES = 2;
const twz_error_code OUT_OF_NAMES = 3;
const twz_error_code UNAVAILABLE = 4;
const twz_error_code BUSY = 5;
const twz_error_code NOT_CONNECTED = 6;
const twz_error_code UNREACHABLE = 7;
const twz_error_code REFUSED = 8;
const twz_error_code NON_ATOMIC = 9;

// Naming
const twz_error_code NOT_FOUND = 1;
const twz_error_code ALREADY_EXISTS = 2;
const twz_error_code WRONG_NAME_KIND = 3;
const twz_error_code ALREADY_BOUND = 4;
const twz_error_code INVALID_NAME = 5;
const twz_error_code LINK_LOOP = 6;
const twz_error_code NOT_EMPTY = 7;

// Object
const twz_error_code MAPPING_FAILED = 1;
const twz_error_code NOT_MAPPED = 2;
const twz_error_code INVALID_FOTE = 3;
const twz_error_code INVALID_PTR = 4;
const twz_error_code INVALID_META = 5;
const twz_error_code BASETYPE_MISMATCH = 6;
const twz_error_code NO_SUCH_OBJECT = 7;

// I/O
const twz_error_code OTHER_IO_ERROR = 1;
const twz_error_code DATA_LOSS = 2;
const twz_error_code DEVICE_ERROR = 3;
const twz_error_code SEEK_FAILED = 4;
const twz_error_code RESET = 5;

// Security
const twz_error_code INVALID_KEY = 1;
const twz_error_code INVALID_SCHEME = 2;
const twz_error_code SIGNATURE_MISMATCH = 3;
const twz_error_code GATE_DENIED = 4;
const twz_error_code INVALID_GATE = 5;

// Kinds
const twz_error_category UNCATEGORIZED_ERROR = 0;
const twz_error_category GENERIC_ERROR = 1;
const twz_error_category ARGUMENT_ERROR = 2;
const twz_error_category RESOURCE_ERROR = 3;
const twz_error_category NAMING_ERROR = 4;
const twz_error_category OBJECT_ERROR = 5;
const twz_error_category IO_ERROR = 6;
const twz_error_category SECURITY_ERROR = 7;

#ifdef __cplusplus
}
#endif
