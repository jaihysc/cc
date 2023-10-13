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

int il2_isjump(IL2Ins ins) {
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
