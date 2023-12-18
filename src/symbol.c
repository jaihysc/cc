#include "symbol.h"

#include "common.h"

ErrorCode symbol_construct(Symbol* sym, const char* token, Type* type) {
	ErrorCode ecode;
	sym->class = sl_normal;

	// Copy token into symbol
	int i = 0;
	while (token[i] != '\0') {
		if (i >= MAX_SYMBOL_LEN) {
			sym->token[0] = '\0';
			return ec_symbol_nametoolong;
		}
		sym->token[i] = token[i];
		++i;
	}
	sym->token[i] = '\0';

	if ((ecode = type_copy(type, &sym->type)) != ec_noerr) return ecode;
	sym->valcat = vc_none;
	return ec_noerr;
}

void symbol_destruct(Symbol* sym) {
	type_destruct(&sym->type);
}

void symbol_sl_access(Symbol* sym, Symbol* ptr, Symbol* idx) {
	ASSERT(sym != NULL, "Symbol is null");
	sym->class = sl_access;
	sym->ptr = ptr;
	sym->ptr_idx = idx;
}

SymbolClass symbol_class(Symbol* sym) {
	return sym->class;
}

char* symbol_token(Symbol* sym) {
	ASSERT(sym != NULL, "Symbol is null");
	return sym->token;
}

Type* symbol_type(Symbol* sym) {
	ASSERT(sym != NULL, "Symbol is null");
	return &sym->type;
}

ValueCategory symbol_valcat(Symbol* sym) {
	ASSERT(sym != NULL, "Symbol is null");
	return sym->valcat;
}

void symbol_set_valcat(Symbol* sym, ValueCategory valcat) {
	ASSERT(sym != NULL, "Symbol is null");
	sym->valcat = valcat;
}

Symbol* symbol_ptr_sym(Symbol* sym) {
	ASSERT(sym != NULL, "Symbol is null");
	return sym->ptr;
}

Symbol* symbol_ptr_index(Symbol* sym) {
	ASSERT(sym != NULL, "Symbol is null");
	return sym->ptr_idx;
}

int symbol_is_constant(Symbol* sym) {
	ASSERT(sym != NULL, "Symbol is null");
	return '0' <= sym->token[0] && sym->token[0] <= '9';
}
