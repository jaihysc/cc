/* Error code for indicating errors */
#ifndef ERRORCODE_H
#define ERRORCODE_H

/* pbufexceed: Parser buffer exceeded
   fileposfailed: Change file position indicator failed */
#define ERROR_CODES                  \
    ERROR_CODE(noerr)                \
                                     \
    ERROR_CODE(lexer_tokbufexceed)   \
    ERROR_CODE(lexer_fopenfail)      \
                                     \
    ERROR_CODE(symbol_nametoolong)   \
                                     \
    ERROR_CODE(symtab_dupname)       \
                                     \
    ERROR_CODE(tnode_childexceed)    \
                                     \
    ERROR_CODE(badalloc)             \
    ERROR_CODE(badclioption)         \
    ERROR_CODE(scopedepexceed)       \
    ERROR_CODE(scopelenexceed)       \
    ERROR_CODE(syntaxerr)            \
    ERROR_CODE(writefailed)          \
    ERROR_CODE(fileposfailed)

/* Should always be initialized to ec_noerr */
/* Since functions will only sets if error occurred */
#define ERROR_CODE(name__) ec_ ## name__,
typedef enum {ERROR_CODES} ErrorCode;
#undef ERROR_CODE

/* Returns error code name as string */
const char* ec_str(ErrorCode ecode);

#endif
