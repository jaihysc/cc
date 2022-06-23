/* System V specifics */
#ifndef SYSV_H
#define SYSV_H

typedef struct {
    int i_int_loc; /* Next location for passing integer */
} CallData;

static void call_construct(CallData* dat) {
    *dat = (CallData){0};
}

/* Returns 1 if the provided location must be saved by the caller, 0 if does
   not */
static int call_caller_save(Location loc) {
    switch (loc) {
        case loc_a:
        case loc_c:
        case loc_d:
        case loc_sp:
        case loc_si:
        case loc_di:
        case loc_8:
        case loc_9:
        case loc_10:
        case loc_11:
            return 1;
        default:
            return 0;
    }
}

/* Returns 1 if the provided location must be saved by the callee, 0 if does
   not */
static int call_callee_save(Location loc) {
    switch (loc) {
        case loc_bp:
        case loc_b:
        case loc_12:
        case loc_13:
        case loc_14:
        case loc_15:
            return 1;
        default:
            return 0;
    }
}

/* Call this function with the arguments of a function, left to right
   Returns the location which the provided argument should be stored / is
   stored for a function call */
static Location call_arg_loc(CallData* dat, const Symbol* sym) {
    const Location int_loc[] = {loc_di, loc_si, loc_d, loc_c, loc_8, loc_9};
    ASSERT(dat->i_int_loc < ARRAY_SIZE(int_loc),
            "No registers left to pass argument");

    return int_loc[dat->i_int_loc++];
}

/* Returns where the provided symbol should be placed to return it from a
   function */
static Location call_ret_loc(const Symbol* sym) {
    if (reg_has_size(symbol_bytes(sym))) {
        return loc_a;
    }
    else {
        ASSERT(0, "Unimplemented");
    }
}

#endif
