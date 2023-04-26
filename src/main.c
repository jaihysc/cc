/* Entry point of compiler */

#include "common.h"

#include "globals.h"
#include "il2gen.h"
#include "parser.h"

typedef struct
{
	/* Heap allocated paths for files */
	char* input_path;
	char* output_path;
} Flags;

static ErrorCode flags_construct(Flags* f) {
	cmemzero(f, sizeof(Flags));
	return ec_noerr;
}

static ErrorCode flags_destruct(Flags* f) {
	if (f->input_path != NULL) cfree(f->input_path);
	if (f->output_path != NULL) cfree(f->output_path);
	return ec_noerr;
}

/* The index of the option's string in the string array
   is the index of the pointer for the variable corresponding to the option

   SWITCH_OPTION(option string, variable to set)
   Order by option string, see strbinfind for ordering requirements */
#define SWITCH_OPTIONS                                                    \
	SWITCH_OPTION(-dprint-cfg, g_debug_print_cfg)                         \
	SWITCH_OPTION(-dprint-parse-recursion, g_debug_print_parse_recursion) \
	SWITCH_OPTION(-dprint-symtab, g_debug_print_symtab)                   \
	SWITCH_OPTION(-dprint-tree, g_debug_print_tree)

#define SWITCH_OPTION(str__, var__) #str__,
const char* option_switch_str[] = {SWITCH_OPTIONS};
#undef SWITCH_OPTION
#define SWITCH_OPTION(str__, var__) &var__,
int* option_switch_value[] = {SWITCH_OPTIONS};
#undef SWITCH_OPTION

/* Parses cli args and processes them */
static ErrorCode handle_cli_arg(Flags* f, int argc, char** argv) {
	ErrorCode ecode = ec_noerr;

	/* Skip first argv since it is path */
	for (int i = 1; i < argc; ++i) {
		/* Handle switch options */
		int i_switch = strbinfind(argv[i], strlength(argv[i]), option_switch_str, ARRAY_SIZE(option_switch_str));
		if (i_switch >= 0) {
			*option_switch_value[i_switch] = 1;
			continue;
		}

		if (strequ(argv[i], "-o")) {
			/* Output path */
			if (f->output_path != NULL) {
				ERRMSG("Only one output file can be specified\n");
				ecode = ec_badclioption;
				break;
			}

			++i;
			if (i >= argc) {
				ERRMSG("Expected output file path after -o\n");
				ecode = ec_badclioption;
				break;
			}
			f->output_path = cmalloc((strlength(argv[i]) + 1) * sizeof(char));
			if (f->output_path == NULL) {
				ecode = ec_badalloc;
				break;
			}
			strcopy(argv[i], f->output_path);
		}
		else {
			/* Input path */
			if (f->input_path != NULL) {
				/* Incorrect flag */
				ERRMSGF("Unrecognized argument" TOKEN_COLOR " %s\n", argv[i]);
				ecode = ec_badclioption;
				break;
			}
			f->input_path = cmalloc((strlength(argv[i]) + 1) * sizeof(char));
			if (f->input_path == NULL) {
				ecode = ec_badalloc;
				break;
			}
			strcopy(argv[i], f->input_path);
		}
	}

	return ecode;
}

int main(int argc, char** argv) {
	ErrorCode ecode;

	/* Process flags */

	Flags flags;
	if ((ecode = flags_construct(&flags)) != ec_noerr) goto exit;

	if ((ecode = handle_cli_arg(&flags, argc, argv)) != ec_noerr) goto exit1;

	if (flags.input_path == NULL) {
		ERRMSG("No input file\n");
		goto exit1;
	}
	if (flags.output_path == NULL) {
		/* Default to opening imm2 */
		const char* path = "imm2";

		flags.output_path = cmalloc((strlength(path) + 1) * sizeof(char));
		if (flags.output_path == NULL) {
			ecode = ec_badalloc;
			goto exit1;
		}
		strcopy(path, flags.output_path);
	}

	/* Parse source code */

	Lexer lex;
	if ((ecode = lexer_construct(&lex, flags.input_path)) != ec_noerr) {
		ERRMSGF("Failed to open input file" TOKEN_COLOR " %s\n", flags.input_path);
		goto exit1;
	}

	Symtab symtab;
	if ((ecode = symtab_construct(&symtab)) != ec_noerr) goto exit2;

	Tree tree;
	if ((ecode = tree_construct(&tree)) != ec_noerr) goto exit3;

	Parser p;
	if ((ecode = parser_construct(&p, &lex, &symtab, &tree)) != ec_noerr) goto exit4;

	if ((ecode = symtab_push_scope(&symtab)) != ec_noerr) goto exit4;

	ecode = parse_translation_unit(&p);
	if (ecode != ec_noerr) {
		lexer_print_location(&lex);
		ERRMSG("Failed to build Tree\n");
		goto exit4;
	}

	symtab_pop_scope(&symtab);
	ASSERT(symtab.scopes_size == 0, "Scopes not empty on parse end");

	if (g_debug_print_tree) {
		LOG("Remaining ");
		debug_print_tree(&tree);
	}

	/* Generate IL2 */

	Cfg cfg;
	if ((ecode = cfg_construct(&cfg)) != ec_noerr) goto exit4;

	IL2Gen il2;
	if ((ecode = il2_construct(&il2, &cfg, &symtab, &tree)) != ec_noerr) goto exit5;

	if ((ecode = symtab_push_scope(&symtab)) != ec_noerr) goto exit5;

	ecode = il2_gen(&il2);
	if (ecode != ec_noerr) {
		ERRMSG("Failed to generate IL2\n");
		goto exit5;
	}

	symtab_pop_scope(&symtab);
	for (int i = 0; i < sc_count; ++i) {
		ASSERTF(vec_size(&symtab.cat[i]) == 0, "Symbol category stack %d not empty on parse end", i);
	}

	if (g_debug_print_cfg) {
		debug_print_cfg(&cfg);
	}

	if ((ecode = il2_write(&il2, flags.output_path)) != ec_noerr) goto exit5;

exit5:
	cfg_destruct(&cfg);
exit4:
	tree_destruct(&tree);
exit3:
	symtab_destruct(&symtab);
exit2:
	lexer_destruct(&lex);
exit1:
	flags_destruct(&flags);
exit:
	if (ecode != ec_noerr) {
		ERRMSGF("Error during parsing: %d %s\n", ecode, ec_str(ecode));
	}
	return ecode;
}
