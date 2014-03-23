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
  item_null
} ItemType;

typedef struct {
  ItemType type;
  ItemValue value;
} Item;

Item from_json(char *json_str);
char *to_json(Item item);
