/* Assembly generator, struct ErrorCode */
#ifndef ASMGEN_ERRORCODE_H
#define ASMGEN_ERRORCODE_H

#define ERROR_CODES            \
    ERROR_CODE(noerr)          \
    ERROR_CODE(insbufexceed)   \
    ERROR_CODE(argbufexceed)   \
    ERROR_CODE(scopelenexceed) \
    ERROR_CODE(invalidins)     \
    ERROR_CODE(invalidinsop)   \
    ERROR_CODE(invalidlabel)   \
    ERROR_CODE(badargs)        \
    ERROR_CODE(badmain)        \
    ERROR_CODE(writefailed)    \
    ERROR_CODE(seekfailed)     \
    ERROR_CODE(outofmemory)    \
    ERROR_CODE(unknownsym)

#define ERROR_CODE(name__) ec_ ## name__,
typedef enum {ERROR_CODES} ErrorCode;
#undef ERROR_CODE
#define ERROR_CODE(name__) #name__,
char* errcode_str[] = {ERROR_CODES};
#undef ERROR_CODE
#undef ERROR_CODES

#endif
