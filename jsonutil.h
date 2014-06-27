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

#define copy_str_item(str) ((json_Item){ .type = item_string, .value.string = strdup(str) })
#define wrap_str_item(str) ((json_Item){ .type = item_string, .value.string = str })
#define new_arr_item() ((json_Item){ .type = item_array, .value.array = CArrayNew(0, sizeof(json_Item)) })
#define num_item(num) ((json_Item){ .type = item_number, .value.number = num })
#define true_item ((json_Item){ .type = item_true })
#define false_item ((json_Item){ .type = item_false })

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
