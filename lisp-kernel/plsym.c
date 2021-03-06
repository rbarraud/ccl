/*
 * Copyright 1994-2009 Clozure Associates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "lispdcmd.h"

void
describe_symbol(LispObj sym)
{
  lispsymbol *rawsym = (lispsymbol *)ptr_from_lispobj(untag(sym));
  LispObj function = rawsym->fcell;
#ifdef fulltag_symbol
  sym += (fulltag_symbol-fulltag_misc);
#endif
  Dprintf("Symbol %s at #x%llX", print_lisp_object(sym), (long long)sym);
  Dprintf("  value    : %s", print_lisp_object(rawsym->vcell));
  if (function != nrs_UDF.vcell) {
    Dprintf("  function : %s", print_lisp_object(function));
  }
}
  
int
compare_lisp_string_to_c_string(lisp_char_code *lisp_string,
                                char *c_string,
                                natural n)
{
  natural i;
  for (i = 0; i < n; i++) {
    if (lisp_string[i] != (lisp_char_code)(c_string[i])) {
      return 1;
    }
  }
  return 0;
}

/*
  Walk the heap until we find a symbol
  whose pname matches "name".  Return the 
  tagged symbol or NULL.
*/

LispObj
find_symbol_in_range(LispObj *start, LispObj *end, char *name)
{
  LispObj header, tag;
  int n = strlen(name);
  char *s = name;
  lisp_char_code *p;
  while (start < end) {
    header = *start;
    tag = fulltag_of(header);
    if (header_subtag(header) == subtag_symbol) {
      LispObj 
        pname = deref(ptr_to_lispobj(start), 1),
        pname_header = header_of(pname);
      if ((header_subtag(pname_header) == subtag_simple_base_string) &&
          (header_element_count(pname_header) == n)) {
        p = (lisp_char_code *) ptr_from_lispobj(pname + misc_data_offset);
        if (compare_lisp_string_to_c_string(p, s, n) == 0) {
          return (ptr_to_lispobj(start))+fulltag_misc;
        }
      }
    }
    if (nodeheader_tag_p(tag)) {
      start += (~1 & (2 + header_element_count(header)));
    } else if (immheader_tag_p(tag)) {
      start = (LispObj *) skip_over_ivector((natural)start, header);
    } else {
      start += 2;
    }
  }
  return (LispObj)NULL;
}

LispObj 
find_symbol(char *name)
{
  area *a =  ((area *) (ptr_from_lispobj(lisp_global(ALL_AREAS))))->succ;
  area_code code;
  LispObj sym = 0;

  while ((code = a->code) != AREA_VOID) {
    if ((code == AREA_STATIC) ||
        (code == AREA_DYNAMIC) ||
        (code == AREA_MANAGED_STATIC)) {
      sym = find_symbol_in_range((LispObj *)(a->low), (LispObj *)(a->active), name);
      if (sym) {
        break;
      }
    }
    a = a->succ;
  }
  return sym;
}

    
void 
plsym(ExceptionInformation *xp, char *pname) 
{
  natural address = 0;

  address = find_symbol(pname);
  if (address == 0) {
    Dprintf("Can't find symbol.");
    return;
  }
  
  if ((fulltag_of(address) == fulltag_misc) &&
      (header_subtag(header_of(address)) == subtag_symbol)){
    describe_symbol(address);
  } else {
    fprintf(dbgout, "Not a symbol.\n");
  }
  return;
}

