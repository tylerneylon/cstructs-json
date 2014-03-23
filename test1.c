// test1.c

#include "cjson.h"

#include <stdio.h>

static char *item_type_names[] = {
  "item_string",
  "item_number",
  "item_object",
  "item_array",
  "item_true",
  "item_false",
  "item_null"
};

int main() {
  char *json_str = "\"hello\"";
  printf("String to parse is:\n%s\n", json_str);
  Item item = from_json(json_str);

  printf("Parsed result has type %s\n", item_type_names[item.type]);
  if (item.type == item_string) {
    printf("Value is:\n%s\n", item.value.string);
  }

  return 0;
}
