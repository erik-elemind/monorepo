#ifndef _ERR_CODES_H_
#define _ERR_CODES_H_

typedef enum error_t
{
    ERROR_NONE = 0,
    ERROR_INDEX_OUT_OF_BOUNDS = 1,
    ERROR_CMD_CODE_NOT_FOUND = 2,
    ERROR_INVALID_CRC = 3,
    ERROR_INVALID_CHANNEL = 4,
    ERROR_DOES_NOT_EXIST = 6,
} error_t;

#endif //_ERR_CODES_H_
