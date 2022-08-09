/* Lexer / token handling */

/* Returns 1 is considered whitespace */
static int iswhitespace(char c) {
    switch (c) {
        case ' ':
        case '\t':
        case '\n':
            return 1;
        default:
            return 0;
    }
}

/* If character is part of a punctuator */
static int isofpunctuator(char c) {
    switch (c) {
        case '[': case ']': case '(': case ')': case '{': case '}':
        case '.': case '-': case '+': case '&': case '*': case '~':
        case '!': case '/': case '%': case '<': case '>': case '=':
        case '^': case '|': case '?': case ':': case ';': case ',':
        case '#':
            return 1;
        default:
            return 0;
    }
}

/* C keyword handling */

/* Returns 1 if string is considered as a keyword, 0 otherwise */
static int tok_iskeyword(const char* token) {
    const char* token_keyword[] = {
        "_Bool", "_Complex", "_Imaginary",
        "auto", "break", "case", "char", "const", "continue", "default", "do",
        "double", "else", "enum", "extern", "float", "for", "goto", "if",
        "inline", "int", "long", "register", "restrict", "return", "short",
        "signed", "sizeof", "static", "struct", "switch", "typedef", "union",
        "unsigned", "void", "volatile", "while"
    };
    return strbinfind(
            token,
            strlength(token),
            token_keyword,
            ARRAY_SIZE(token_keyword)) >= 0;
}

/* Returns 1 if string is considered a unary operator */
static int tok_isunaryop(const char* str) {
    if (strlength(str) != 1) {
        return 0;
    }

    switch (str[0]) {
        case '&': case '*': case '+': case '-': case '~': case '!':
            return 1;
        default:
            return 0;
    }
}

/* TODO why declare these in global scope? Move them to be a part of their function */
const char* token_assignment_operator[] = {
    /* See strbinfind for ordering requirements */
    "%=", "&=", "*=", "+=", "-=", "/=", "<<=", "=", ">>=", "^=", "|="
};
/* Returns 1 if token is considered an assignment operator */
static int tok_isassignmentop(const char* token) {
    return strbinfind(
            token,
            strlength(token),
            token_assignment_operator,
            ARRAY_SIZE(token_assignment_operator)) >= 0;
}

const char* token_storage_class_keyword[] = {
    /* See strbinfind for ordering requirements */
    "auto",
    "extern",
    "register",
    "static",
    "typedef"
};
/* Returns 1 if token c string is a storage class keyword, 0 otherwise */
static int tok_isstoreclass(const char* str) {
    return strbinfind(
            str,
            strlength(str),
            token_storage_class_keyword,
            ARRAY_SIZE(token_storage_class_keyword)) >= 0;
}

const char* token_type_keyword[] = {
    /* See strbinfind for ordering requirements */
    "char",
    "double",
    "float",
    "int",
    "long",
    "short",
    "signed",
    "unsigned",
    "void",
};
/* Returns 1 if token c string is a type specifier keyword, 0 otherwise */
static int tok_istypespec(const char* str) {
    return strbinfind(
            str,
            strlength(str),
            token_type_keyword,
            ARRAY_SIZE(token_type_keyword)) >= 0;
}

const char* token_type_qualifier_keyword[] = {
    /* See strbinfind for ordering requirements */
    "const",
    "restrict",
    "volatile"
};
/* Returns 1 if token c string is a type qualifier keyword, 0 otherwise */
static int tok_istypequal(const char* str) {
    return strbinfind(
            str,
            strlength(str),
            token_type_qualifier_keyword,
            ARRAY_SIZE(token_type_qualifier_keyword)) >= 0;
}

/* Returns 1 if token c string is a function specifier keyword, 0 otherwise */
static int tok_isfuncspec(const char* str) {
    return strequ(str, "inline");
}

/* Returns 1 if token c string is an identifier */
static int tok_isidentifier(const char* str) {
    if (tok_iskeyword(str)) {
        return 0;
    }

    char c = str[0];
    if (c == '\0') {
        return 0;
    }
    int is_nondigit = 0;
    if ('a' <= c && c <= 'z') {
        is_nondigit = 1;
    }
    else if ('A' <= c && c <= 'Z') {
        is_nondigit = 1;
    }
    else if (c == '_') {
        is_nondigit = 1;
    }
    if (!is_nondigit) {
        return 0;
    }

    int i = 1;
    while ((c = str[i]) != '\0') {
        int is_id = 0;
        if ('a' <= c && c <= 'z') {
            is_id = 1;
        }
        else if ('A' <= c && c <= 'Z') {
            is_id = 1;
        }
        else if (c == '_') {
            is_id = 1;
        }
        else if ('0' <= c && c <= '9') {
            is_id = 1;
        }
        if (!is_id) {
            return 0;
        }
        ++i;
    }
    return 1;
}

/* Reads in a character from input
   Does not advance read position */
static char read_char(Parser* p) {
    /* Perhaps this can be a buffer in the future to reduce system calls */
    char c = (char)getc(p->rf);

    /* Handle line marker from preprocessor*/
    if (p->char_num == 1) {
        while (c == '#') {
            getc(p->rf); /* Consume space */

            /* Line number */
            int line_num = 0;
            while ((c = (char)getc(p->rf)) != ' ') {
                line_num *= 10;
                line_num += c - '0';
            }
            p->line_num = line_num;

            /* Consume linemarker */
            while ((c = (char)getc(p->rf)) != '\n');
            c = (char)getc(p->rf); /* First char of next line */
        }
    }

    fseek(p->rf, -1, SEEK_CUR);
    return c;
}

/* Reads in a character from input 1 ahead
   of character from read_char
   Does not advance read position */
static char read_char_next(Parser* p) {
    getc(p->rf);
    char c = (char)getc(p->rf);
    fseek(p->rf, -2, SEEK_CUR);
    return c;
}


/* Advances read position to next char */
static void consume_char(Parser* p) {
    /* Move the file position forwards */
    char c = (char)getc(p->rf);

    if (c == '\n') {
        ++p->line_num;
        p->char_num = 1;
    }
    else {
        ++p->char_num;
    }
}

/* Indicates the pointed to token is no longer in use */
static void consume_token(char* token) {
    token[0] = '\0';
    if (g_debug_print_parse_recursion) {
        LOG("^Consumed\n");
    }
}

/* Reads a null terminated token
   Returns pointer to the token, the token is blank (just a null terminator)
   if end of file is reached or error happened */
static char* read_token(Parser* p) {
    /* Has cached token */
    if (p->read_buf[0] != '\0') {
        if (g_debug_print_parse_recursion) {
            LOGF("%s ^Cached\n", p->read_buf);
        }
        return p->read_buf;
    }

    char c = read_char(p);
    /* Skip leading whitespace */
    while (iswhitespace(c = read_char(p))) {
        consume_char(p);
    }

    /* Handle punctuators
       sufficient space guaranteed to hold all punctuators*/
    ASSERT(MAX_TOKEN_LEN >= 4,
            "Max token length must be at least 4 to hold C operators");
    if (isofpunctuator(c)) {
        char cn = read_char_next(p);
        /* <= >= == != *= /= %= += -= &= ^= |= */
        if (cn == '=') {
            p->read_buf[0] = c;
            p->read_buf[1] = '=';
            p->read_buf[2] = '\0';
            consume_char(p);
            consume_char(p);
            goto exit;
        }

        /* Handle double char punctuators
           + ++ - -- & && | || */
        char chars[] = {'+', '-', '&', '|'};
        for (int i = 0; i < ARRAY_SIZE(chars); ++i) {
            if (c == chars[i]) {
                consume_char(p);
                p->read_buf[0] = chars[i];
                if (cn == chars[i]) {
                    consume_char(p);
                    p->read_buf[1] = chars[i];
                    p->read_buf[2] = '\0';
                }
                else {
                    p->read_buf[1] = '\0';
                }
                goto exit;
            }
        }

        /* TODO For now, treat everything else as token */
        p->read_buf[0] = c;
        p->read_buf[1] = '\0';
        consume_char(p);
        goto exit;
    }

    /* Handle others */
    int i = 0;
    while (c != EOF) {
        if (iswhitespace(c) || isofpunctuator(c)) {
            break;
        }

        if (i >= MAX_TOKEN_LEN) {
            parser_set_error(p, ec_tokbufexceed);
            break; /* Since MAX_TOKEN_LEN excludes null terminator, we can break and add null terminator */
        }
        p->read_buf[i] = c;
        consume_char(p);
        c = read_char(p);
        ++i;
    }
    p->read_buf[i] = '\0';

exit:
    if (g_debug_print_parse_recursion) {
        LOGF("%s\n", p->read_buf);
    }

    return p->read_buf;
}




