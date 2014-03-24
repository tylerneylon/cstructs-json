// cjson_test.c
//
// For testing us some cjson.
//

#include "cjson.h"

#include "ctest.h"
#include <stdio.h>
#include <string.h>

#define true 1
#define false 0

#define array_size(x) (sizeof(x) / sizeof(x[0]))

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
  // This str_val is used for literals; it's also overwritten for strings and errors.
  char *str_val = (item.type == item_true ? "true" : (item.type == item_false ? "false" : "null"));
  switch (item.type) {
    case item_string:
    case item_error:
      str_val = item.value.string;  // Fall through purposefully here.
    case item_true:
    case item_false:
    case item_null:
      printf("%s\n", str_val);
      break;
    case item_number:
      printf("%g\n", item.value.number);
      break;
    case item_array:
      printf(item.value.array->count ? "[\n" : "[");
      CArrayFor(Item *, subitem, item.value.array) {
        print_item(*subitem, indent + 2, true /* indent first line */);
      }
      if (item.value.array->count) indent_by(indent);
      printf("]\n");
      break;
    case item_object:
      printf(item.value.object->count ? "{\n" : "{");
      CMapFor(pair, item.value.object) {
        indent_by(indent + 2);
        printf("%s : ", (char *)pair->key);
        print_item(*(Item *)pair->value, indent + 2, false /* indent first line */);
      }
      if (item.value.object->count) indent_by(indent);
      printf("}\n");
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

int old_main() {
  char *json_str = "\"hello\"";

  // Test strings.
  parse_str("\"hello\"");

  // Test arrays.
  parse_str(" [ \"abc\", \n \"def\" ] ");
  parse_str("[\"abc\" \"def\"]");

  // Test objects.
  parse_str("{\"a\": \"def\"}");
  parse_str("{ \"str\" : \"ing\" , \"arr\":[\"a\", []]}");
  parse_str("{ \"str\" : \"ing\" , \"arr\":[\"a\", []}");

  // Test literals.
  parse_str("true");
  parse_str("troo");
  parse_str("false");
  parse_str("null");
  parse_str("[true, \"hi\", {\"apple\": true, \"banana\": false, \"robot\": [null]}]");

  // Test numbers.
  parse_str("0");
  parse_str("1");
  parse_str("12");
  parse_str("3.14");
  parse_str("-0.55");
  parse_str("1e2");
  parse_str("1E-1");
  parse_str("2e0");
  parse_str("-2E+20");
  parse_str("[1, 2, 3.14, 2e2, {\"hi\": -100}]");

  // Number error cases.
  parse_str("-");
  parse_str("1e");
  parse_str("01");
  parse_str("+3");
  parse_str("0e-");

  // Some tests to check that printing looks good.
  parse_str("[]");
  parse_str("[[], [], [[]]]");
  parse_str("{\"a\": [], \"b\": {}, \"c\": {\"d\": \"e\"}}");

  // TODO After all value types can be parsed, check error-reporting on an invalid first char.
  //      e.g. "[,]" or "gru".

  printf("\n");
  return 0;
}

typedef struct {
  char *str;
  Item item;
} StringAndItem;

static void check_parse(StringAndItem str_and_item) {
  test_printf("About to parse:\n%s\n", str_and_item.str);
  Item parsed_item = from_json(str_and_item.str);
  test_printf("Result has type %s\n", item_type_names[parsed_item.type]);
  Item expected_item = str_and_item.item;
  test_that(parsed_item.type == expected_item.type);

  // We don't check error strings here.
  if (parsed_item.type == item_error) return;

  // TODO test_print out a nice-looking version of the parsed result
  printf("Result:\n");
  print_item(parsed_item, 0 /* indent */, false /* indent first line */);

  // We'll only get here if the item types match.
  ItemValue parsed_val = parsed_item.value;
  ItemValue expected_val = expected_item.value;
  switch (expected_item.type) {
    case item_error:
    case item_string:
      test_that(strcmp(parsed_val.string, expected_val.string) == 0);
      break;
    case item_number:
      test_that(parsed_val.number == expected_val.number);
      break;
    default:
      test_failed("unknown item type");
  }
}

int test_parse_number() {
  StringAndItem test_data[] = {
    // Non-error cases.
    {"0", {.type = item_number, .value.number = 0.0}},
    {"1", {.type = item_number, .value.number = 1}},
    {"12", {.type = item_number, .value.number = 12}},
    {"3.14", {.type = item_number, .value.number = 3.14}},
    {"-0.55", {.type = item_number, .value.number = -0.55}},
    {"1e2", {.type = item_number, .value.number = 1e2}},
    {"1E-1", {.type = item_number, .value.number = 1E-1}},
    {"2e0", {.type = item_number, .value.number = 2e0}},
    {"-2E+20", {.type = item_number, .value.number = -2E+20}},

    // Error cases.
    {"-", {.type = item_error}},
    {"1e", {.type = item_error}},
    // {"01", {.type = item_error}},  // This is a weird case; if we allow tails, it's legit.
    {"+3", {.type = item_error}},
    {"0e-", {.type = item_error}}
  };
  for (int i = 0; i < array_size(test_data); ++i) check_parse(test_data[i]);
  return test_success;
}

int main(int argc, char **argv) {
  start_all_tests(argv[0]);
  run_tests(
    test_parse_number
  );
  return end_all_tests();
}