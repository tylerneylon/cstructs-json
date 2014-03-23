// test1.c

#include "cjson.h"

#include <stdio.h>

#define true 1
#define false 0

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

static inline void indent_by(int indent) {
  for (int i = 0; i < indent; ++i) printf(" ");
}

void print_item(Item item, int indent, int indent_first_line) {
  if (indent_first_line) indent_by(indent);
  switch (item.type) {
    case item_string:
    case item_error:
      printf("%s\n", item.value.string);
      break;
    case item_array:
      if (item.value.array->count == 0) {
        printf("[]\n");
      } else {
        if (!indent_first_line) {
          printf("\n");
          indent_by(indent);
        }
        printf("[\n");
        int indent_first_line = true;
        CArrayFor(Item *, subitem, item.value.array) {
          print_item(*subitem, indent + 2, indent_first_line);
        }
        indent_by(indent);
        printf("]\n");
      }
      break;
    case item_object:
      if (item.value.object->count == 0) {
        printf("{}\n");
      } else {
        if (!indent_first_line) {
          printf("\n");
          indent_by(indent);
        }
        printf("{\n");
        int indent_first_line = false;
        indent_by(indent);
        CMapFor(pair, item.value.object) {
          indent_by(indent + 2);
          printf("%s : ", (char *)pair->key);
          print_item(*(Item *)pair->value, indent + 2, indent_first_line);
        }
        printf("}\n");
      }
      break;
    default:
      printf("(printing this type is not yet implemented)\n");
      break;
  }
}

void parse_str(char *str) {
  printf("\nString to parse is:\n%s\n", str);
  Item item = from_json(str);

  printf("Parsed result has type %s\n", item_type_names[item.type]);

  int indent = 0;
  int indent_first_line = true;
  print_item(item, indent, indent_first_line);
}

int main() {
  char *json_str = "\"hello\"";
  parse_str("\"hello\"");
  parse_str(" [ \"abc\", \n \"def\" ] ");
  parse_str("[\"abc\" \"def\"]");
  parse_str("{\"a\": \"def\"}");
  parse_str("{ \"str\" : \"ing\" , \"arr\":[\"a\", []]}");
  parse_str("{ \"str\" : \"ing\" , \"arr\":[\"a\", []}");

  // TODO After all value types can be parsed, check error-reporting on an invalid first char.
  //      e.g. "[,]" or "gru".

  printf("\n");
  return 0;
}
