/* Compiler data structure + functions */

/* Handles format specifier $d
   Prints int as string
   Returns 1 if successful, 0 if not */
static int parser_output_ild(Parser* p, int i) {
    return fprintf(p->of, "%d", i) >= 0;
}

/* Handles format specifier $i
   Prints char* as string
   Returns 1 if successful, 0 if not */
static int parser_output_ili(Parser* p, const char* str) {
    return fprintf(p->of, "%s", str) >= 0;
}

/* Handles format specifier $s
   Prints Symbol as string
   Returns 1 if successful, 0 if not */
static int parser_output_ils(Parser* p, SymbolId sym_id) {
    int scope_num = symtab_get_scope_num(p, sym_id);
    /* symtab_add sets scope num to 0 for global scope */
    if (scope_num > 0) {
        return fprintf(p->of, "_Z%d%s",
            scope_num, symtab_get_token(p, sym_id)) >= 0;
    }
    else {
        /* No prefix for global scope as constants sit in global scope */
        return fprintf(p->of, "%s", symtab_get_token(p, sym_id)) >= 0;
    }
}

/* Handles format specifier $t
   Prints Symbol Type as string
   Returns 1 if successful, 0 if not */
static int parser_output_ilt(Parser* p, SymbolId sym_id) {
    Type type = symtab_get_type(p, sym_id);
    if (fprintf(p->of, "%s", type_specifiers_str(type_typespec(&type))) < 0)
        goto error;
    for (int i = 0; i < type_pointer(&type); ++i) {
        if (fprintf(p->of, "*") < 0) goto error;
    }
    for (int i = 0; i < type_dimension(&type); ++i) {
        fprintf(p->of, "[%d]", type_dimension_size(&type, i));
    }
    return 1;

error:
    return 0;
}

/* Writes provided IL using CUSTOM format string (See below) and va_args
   into output
   Format string: The format specifier is replaced with a result
   Format specifier | Parameter | Result
    $d                int         Integer as string
    $i                char*       String (Think Instruction)
    $s                SymbolId    Name of symbol
    $t                Type        Type as a string
*/
static void parser_output_il(Parser* p, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    /* Scans format string as follows:
        Prints from fmt until format specifier
        Move fmt to after format specifier
        Continue until null terminator */
    int i = 0;
    char c;
    while ((c = fmt[i]) != '\0') {
        if (c == '$') {
            /* Print string from fmt until current position */
            if (fprintf(p->of, "%.*s", i, fmt) < 0) goto error;
            char format_c = fmt[++i];
            ASSERT(format_c != '\0', "Expected format specifier char");

            /* Reposition to keep scanning */
            fmt += i + 1; /* Char after format_c */
            i = 0;

            int success;
            switch (format_c) {
                case 'd':
                    success = parser_output_ild(p, va_arg(args, int));
                    break;
                case 'i':
                    success = parser_output_ili(p, va_arg(args, char*));
                    break;
                case 's':
                    success = parser_output_ils(p, va_arg(args, SymbolId));
                    break;
                case 't':
                    success = parser_output_ilt(p, va_arg(args, SymbolId));
                    break;
                default:
                    ASSERT(0, "Unrecognized format specifier char");
            }
            if (!success) goto error;
        }
        else {
            ++i;
        }
    }
    /* Print out remaining format string */
    if (fprintf(p->of, "%.*s", i, fmt) < 0) goto error;

    goto exit;

error:
    parser_set_error(p, ec_writefailed);

exit:
    va_end(args);
}

