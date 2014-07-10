// jsonutil.h
//
// Home repo: https://github.com/tylerneylon/cstructs-json
//
// Optional utilities that make it easier to use
// cstructs-json in certain cases.
//

#ifndef __JSONUTIL_H__
#define __JSONUTIL_H__

#include "json.h"

// TODO Split these up as cleanly as possible.
//      For now, I'm decided that this means putting the windows
//      versions in their own file and leaving the non-windows stuff here
//      since the non-windows versions are easier to read.
//

// For now the json_Item helper macros do zero bounds or key checking.
// In the future, I'm considering adding an optional flag that could
// control bounds/key-checking at compile time. Perhaps on-by-default is
// a good choice.

// json_Item getter/setters (can be used for either)

#define item_at(arr_itm, idx) (*(json_Item *)CArrayElement((arr_itm).value.array, idx))
#define item_str(str_itm) ((str_itm).value.string)
#define item_num(num_itm) ((num_itm).value.number)
#define str_at(arr_itm, idx) item_str(item_at(arr_itm, idx))

// json_Item setters

#define added_item(arr_itm) (*(json_Item *)CArrayNewElement((arr_itm).value.array))

// json_Item creators
// The item needs to be released iff 'new' or 'copy' is in its name.

#define true_item ((json_Item){ .type = item_true })
#define false_item ((json_Item){ .type = item_false })

#ifndef _WIN32

#define copy_str_item(str) ((json_Item){ .type = item_string, .value.string = strdup(str) })
#define wrap_str_item(str) ((json_Item){ .type = item_string, .value.string = str })

#define new_arr_item() ((json_Item){ .type = item_array, .value.array = CArrayNew(0, sizeof(json_Item)) })
#define num_item(num) ((json_Item){ .type = item_number, .value.number = num })

#else

// Windows versions.

#include <string.h>

__inline json_Item copy_str_item(const char *s) {
  json_Item item;
  item.type = item_string;
  item.value.string = _strdup(s);
  return item;
}

// The input is purposefully non-const as I'd like to
// keep it easy to contain mutable strings. Constant
// strings can be copied instead of wrapped.
__inline json_Item wrap_str_item(char *s) {
  json_Item item;
  item.type = item_string;
  item.value.string = s;
  return item;
}

__inline json_Item new_arr_item() {
  json_Item item;
  item.type = item_array;
  item.value.array = CArrayNew(0, sizeof(json_Item));
  return item;
}

__inline json_Item num_item(double val) {
  json_Item item;
  item.type = item_number;
  item.value.number = val;
  return item;
}

#endif

// TODO Clean up the comments for json_item_has_format.

// Format has the form:
// [<array format>]
// {<object format>}
// ' string
// t true
// f false
// n null
// # number
//
// For now I'm leaving out object formats.
// In the future, they could take a form like this:
// {'key1':<type1>,'key2':<type2>,:<type_for_all_other_keys>}
// also allowing simply {} which doesn't check any types within the object.
//
// In the future, an array can also end with ,* to mean 0 or more elements are allowed;
// otherwise an exact match is expected. For example [#,#] expects exactly
// two numbers, while [#,#,*] expects two numbers either alone or followed by whatever.

int json_item_has_format(json_Item item, char *format);

#endif