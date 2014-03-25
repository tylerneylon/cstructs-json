// cjson.h
//
// A way to serialize / deserialize between cstructs
// objects and strings using JSON.
//

#include "cstructs/cstructs.h"

typedef union {
  char *string;
  long bool;
  CArray array;
  CMap object;
  double number;
} ItemValue;

typedef enum {
  item_string,
  item_number,
  item_object,
  item_array,
  item_true,
  item_false,
  item_null,
  item_error
} ItemType;

typedef struct {
  ItemType type;
  ItemValue value;
} Item;

// Main functions to parse or jsonify.

Item from_json(char *json_str);
char *json_stringify(Item item);

// Helper function to deallocate items.
// release_item is designed for CArray; free_item is designed for CMap.
// They accept a void * type to be a valid releaser for a CMap/CArray.

// This does NOT free the item itself; only its contents, recursively.
void release_item(void *item);

// Frees both the contents and the item itself; does strictly more than release_item.
void free_item(void *item);

// Hash and equality functions for use in a CMap keyed by strings.
int str_hash(void *str_void_ptr);
int str_eq(void *str_void_ptr1, void *str_void_ptr2);
