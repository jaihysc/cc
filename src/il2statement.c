#include "il2statement.h"

#include "common.h"

#define INSTRUCTION(name__) #name__,
const char* il2_string[] = {INSTRUCTIONS};
#undef INSTRUCTION

const char* il2_str(IL2Ins ins) {
	ASSERT(ins >= 0, "Invalid IL2Ins");
	return il2_string[ins];
}

IL2Ins il2_from_str(const char* str) {
	return strbinfind(str, strlength(str), il2_string, ARRAY_SIZE(il2_string));
}

int il2_is_branch(IL2Ins ins) {
	switch (ins) {
	case il2_jmp:
	case il2_jnz:
	case il2_jz:
	case il2_ret:
		return 1;
	default:
		return 0;
	}
}

int il2_is_unconditional_branch(IL2Ins ins) {
	switch (ins) {
	case il2_jmp:
	case il2_ret:
		return 1;
	default:
		return 0;
	}
}

int il2_incfg(IL2Ins ins) {
	switch (ins) {
	case il2_def:
	case il2_func:
	case il2_lab:
		return 0;
	default:
		return 1;
	}
}

IL2Statement il2stat_make0(IL2Ins ins) {
	IL2Statement stat;
	stat.ins = ins;
	stat.argc = 0;
	return stat;
}

IL2Statement il2stat_make1(IL2Ins ins, Symbol* a0) {
	IL2Statement stat;
	stat.ins = ins;
	stat.arg[0] = a0;
	stat.argc = 1;
	return stat;
}

IL2Statement il2stat_make2(IL2Ins ins, Symbol* a0, Symbol* a1) {
	IL2Statement stat;
	stat.ins = ins;
	stat.arg[0] = a0;
	stat.arg[1] = a1;
	stat.argc = 2;
	return stat;
}

IL2Statement il2stat_make(IL2Ins ins, Symbol* a0, Symbol* a1, Symbol* a2) {
	IL2Statement stat;
	stat.ins = ins;
	stat.arg[0] = a0;
	stat.arg[1] = a1;
	stat.arg[2] = a2;
	stat.argc = 3;
	return stat;
}

IL2Ins il2stat_ins(const IL2Statement* stat) {
	ASSERT(stat != NULL, "IL2Statement is null");
	return stat->ins;
}

Symbol* il2stat_arg(const IL2Statement* stat, int i) {
	ASSERT(stat != NULL, "IL2Statement is null");
	ASSERT(i >= 0, "Index out of range");
	ASSERT(i < stat->argc, "Index out of range");
	return stat->arg[i];
}

int il2stat_argc(const IL2Statement* stat) {
	ASSERT(stat != NULL, "IL2Statement is null");
	return stat->argc;
}

int il2stat_use(const IL2Statement* stat, Symbol** used_symbols) {
	switch (il2stat_ins(stat)) {
	case il2_add:
	case il2_ce:
	case il2_cl:
	case il2_cle:
	case il2_cne:
	case il2_div:
	case il2_mod:
	case il2_mul:
	case il2_not:
	case il2_sub:
		used_symbols[0] = il2stat_arg(stat, 1);
		used_symbols[1] = il2stat_arg(stat, 2);
		return 2;

	case il2_mad:
	case il2_mfi:
	case il2_mov:
	case il2_mtc:
	case il2_mti:
		used_symbols[0] = il2stat_arg(stat, 1);
		return 1;

	case il2_ret:
		used_symbols[0] = il2stat_arg(stat, 0);
		return 1;

	case il2_call:
	case il2_def:
	case il2_func:
	case il2_jmp:
	case il2_jnz:
	case il2_jz:
	case il2_lab:
		return 0;
	default:
		ASSERT(0, "Unhandled switch");
	}
	return 0;
}

int il2stat_def(const IL2Statement* stat, Symbol** defined_symbols) {
	switch (il2stat_ins(stat)) {
	case il2_add:
	case il2_ce:
	case il2_cl:
	case il2_cle:
	case il2_cne:
	case il2_div:
	case il2_mad:
	case il2_mfi:
	case il2_mod:
	case il2_mov:
	case il2_mtc:
	case il2_mti:
	case il2_mul:
	case il2_not:
	case il2_sub:
		defined_symbols[0] = il2stat_arg(stat, 0);
		return 1;

	case il2_call:
	case il2_def:
	case il2_func:
	case il2_jmp:
	case il2_jnz:
	case il2_jz:
	case il2_lab:
	case il2_ret:
		return 0;
	default:
		ASSERT(0, "Unhandled switch");
	}
	return 0;
}
