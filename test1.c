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
  "item_null",
  "item_error"
};

void print_item(Item item, int indent) {
  for (int i = 0; i < indent; ++i) printf(" ");
  switch (item.type) {
    case item_string:
      printf("%s\n", item.value.string);
      break;
    case item_array:
      if (item.value.array->count == 0) {
        printf("[]\n");
      } else {
        printf("[\n");
        CArrayFor(Item *, subitem, item.value.array) {
          print_item(*subitem, indent + 2);
        }
        for (int i = 0; i < indent; ++i) printf(" ");
        printf("]\n");
      }
      break;
    default:
      printf("(printing this type is not yet implemented)\n");
      break;
  }
}

void parse_str(char *str) {
  printf("String to parse is:\n%s\n", str);
  Item item = from_json(str);

  printf("Parsed result has type %s\n", item_type_names[item.type]);
  print_item(item, 0);
}

int main() {
  char *json_str = "\"hello\"";
  parse_str("\"hello\"");
  parse_str(" [ \"abc\", \n \"def\" ] ");

  return 0;
}
