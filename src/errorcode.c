#include "errorcode.h"

#define ERROR_CODE(name__) #name__,
const char* errcode_str[] = {ERROR_CODES};
#undef ERROR_CODE

const char* ec_str(ErrorCode ecode) {
    return errcode_str[(int)ecode];
}
