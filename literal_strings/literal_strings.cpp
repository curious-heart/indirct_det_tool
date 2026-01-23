#include "literal_strings/literal_strings.h"

#undef STR_DELCARE_STMT


#define STR_DELCARE_STMT(var_name, lit_str) \
    const char* var_name = lit_str;

LIT_STRINGS_LIST
