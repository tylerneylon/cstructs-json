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

// These are defined in cjson.c but not in the header since they
// exist only when DEBUG is defined, and are only meant for tests.
extern int cjson_net_obj_allocs;
extern int cjson_net_arr_allocs;

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

void print_item_(Item item, int indent, int indent_first_line) {
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
        print_item_(*subitem, indent + 2, true /* indent first line */);
      }
      if (item.value.array->count) indent_by(indent);
      printf("]\n");
      break;
    case item_object:
      printf(item.value.object->count ? "{\n" : "{");
      CMapFor(pair, item.value.object) {
        indent_by(indent + 2);
        printf("%s : ", (char *)pair->key);
        print_item_(*(Item *)pair->value, indent + 2, false /* indent first line */);
      }
      if (item.value.object->count) indent_by(indent);
      printf("}\n");
      break;
  }
}

void parse_str(char *str) {
  printf("\nString to parse is:\n%s\n", str);
  Item item;
  json_parse(str, &item);

  printf("Parsed result has type %s\n", item_type_names[item.type]);

  int indent = 0;
  int indent_first_line = true;
  print_item_(item, indent, indent_first_line);
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
  test_printf("str_and_item.str is at %p\n", str_and_item.str);
  test_printf("first char = 0x%02X\n", *str_and_item.str);
  test_printf("About to parse:\n%s\n", str_and_item.str);
  Item parsed_item;
  json_parse(str_and_item.str, &parsed_item);

  test_printf("Result has raw type %d\n", parsed_item.type);
  test_printf("Result has type %s\n", item_type_names[parsed_item.type]);

  Item expected_item = str_and_item.item;
  test_that(parsed_item.type == expected_item.type);

  // We don't check error strings or values of literals.
  if (parsed_item.type == item_error || parsed_item.type == item_true ||
      parsed_item.type == item_false || parsed_item.type == item_null) {
    return;
  }

  if (parsed_item.type == item_string || parsed_item.type == item_number) {
    if (parsed_item.type == item_string) {
      test_that(strcmp(parsed_item.value.string, expected_item.value.string) == 0);
    } else {
      test_that(parsed_item.value.number == expected_item.value.number);
    }
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

int test_parse_string() {
  StringAndItem test_data[] = {
    // Non-error cases.
    {"\"abc\"", {.type = item_string, .value.string = "abc"}},
    {"\"\\\\\"", {.type = item_string, .value.string = "\\"}},  // "\\" -> \
    {"\"\\\"\"", {.type = item_string, .value.string = "\""}},   // "\"" -> "
    {"\"\n\"", {.type = item_string, .value.string = "\n"}},
    {"\"\"", {.type = item_string, .value.string = ""}},

    // Error cases.
    {"\"", {.type = item_error}},
    {"\"\\\"", {.type = item_error}}
  };
  for (int i = 0; i < array_size(test_data); ++i) check_parse(test_data[i]);
  return test_success;
}

int test_parse_literals() {
  StringAndItem test_data[] = {
    // Non-error cases.
    {"true", {.type = item_true}},
    {"false", {.type = item_false}},
    {"null", {.type = item_null}},

    // Error cases.
    {"troo", {.type = item_error}}
  };
  for (int i = 0; i < array_size(test_data); ++i) check_parse(test_data[i]);
  return test_success;
}

#define parse_to_item(s) \
  test_printf("About to parse:\n%s\n", s); \
  json_parse(s, &item);


int test_parse_arrays() {

  Item item, subitem;

  // Non-error cases.
  parse_to_item("[\"abc\", \"def\"]");
  test_that(item.type == item_array);
  test_that(item.value.array->count == 2);
  subitem = CArrayElementOfType(item.value.array, 1, Item);
  test_that(strcmp(subitem.value.string, "def") == 0);
  release_item(&item);

  parse_to_item(" [ \"abc\", \n \"def\" ] ");
  test_that(item.type == item_array);
  test_that(item.value.array->count == 2);
  subitem = CArrayElementOfType(item.value.array, 1, Item);
  test_that(strcmp(subitem.value.string, "def") == 0);
  release_item(&item);

  parse_to_item("[]");
  test_that(item.type == item_array);
  test_that(item.value.array->count == 0);
  release_item(&item);

  parse_to_item("[[], [[]]]");
  test_that(item.type == item_array);
  test_that(item.value.array->count == 2);
  test_that(CArrayElementOfType(item.value.array, 1, Item).type == item_array);
  release_item(&item);

  // Error cases.
  char *error_strings[] = {
    "[\"abc\" \"def\"]", "[", "[1", "[1,", "[1,]"
  };
  for (int i = 0; i < array_size(error_strings); ++i) {
    parse_to_item(error_strings[i]);
    test_that(item.type == item_error);
    release_item(&item);
  }

  test_that(cjson_net_arr_allocs == 0);

  return test_success;
}

int test_parse_objects() {

  Item item, *subitem;

  // Non-error cases.
  parse_to_item("{\"a\": \"def\"}");
  test_that(item.type == item_object);
  CMapFor(pair, item.value.object) {
    test_that(strcmp((char *)pair->key, "a") == 0);
    subitem = (Item *)pair->value;
    test_that(subitem->type == item_string);
    test_that(strcmp((char *)subitem->value.string, "def") == 0);
  }
  release_item(&item);

  parse_to_item("{ \"str\" : \"ing\" , \"arr\":[\"a\", []]}");
  test_that(item.type == item_object);
  test_that(item.value.object->count == 2);
  test_that(CMapFind(item.value.object, "str") != NULL);
  test_that(CMapFind(item.value.object, "arr") != NULL);
  test_that(CMapFind(item.value.object, "not-a-key") == NULL);

  subitem = (Item *)(CMapFind(item.value.object, "str")->value);
  test_that(subitem->type == item_string);
  test_that(strcmp(subitem->value.string, "ing") == 0);
  release_item(&item);

  parse_to_item("{}");
  test_that(item.type == item_object);
  release_item(&item);

  // Error cases.
  char *error_strings[] = {
    "{", "{, }", "{1: 2}", "{\"a\":}", "{\"b\": 34 \"c\": 35}", "{\"a\" : 1,}", "{\"a\"}"
  };
  for (int i = 0; i < array_size(error_strings); ++i) {
    parse_to_item(error_strings[i]);
    test_that(item.type == item_error);
    release_item(&item);
  }

  test_that(cjson_net_obj_allocs == 0);

  return test_success;
}

int test_parse_mixed() {

  Item item, *subitem;

  parse_to_item("[true, \"hi\", {\"apple\": true, \"banana\": false, \"robot\": [null]}]");
  test_that(item.type == item_array);
  test_that(item.value.array->count == 3);
  subitem = (Item *)CArrayElement(item.value.array, 2);
  test_that(subitem->type == item_object);
  test_that(subitem->value.object->count == 3);

  parse_to_item("{\"a\": [], \"b\": {}, \"c\": {\"d\": \"e\"}}");
  test_that(item.type == item_object);
  test_that(item.value.object->count == 3);

  parse_to_item("{\"abc\": [ 1, 2, {\"defg\": 01}]}");  // 01 is not a valid number in json.
  test_that(item.type == item_error);

  return test_success;
}

#define add_number(arr_item, num) \
  *(Item *)CArrayNewElement(arr_item.value.array) = \
  (Item){ .type = item_number, .value.number = num };

#define new_number(n) \
  (item = malloc(sizeof(Item)), \
  *item = (Item) { .type = item_number, .value.number = n }, \
  item)

// Expects the value to be an Item *.
#define obj_set(obj_item, key, val) \
  CMapSet(obj_item.value.object, key, val)

int test_stringify() {

  // Currently, this test leaks some memory.
  // This is not good practice, but it does work as a test.

  char *str;
  Item *item;

  // Test from items directly.
  Item array_item1 = { .type = item_array, .value.array = CArrayNew(4, sizeof(Item))};
  str = json_stringify(array_item1);
  test_str_eq(str, "[]");

  Item array_item2 = { .type = item_array, .value.array = CArrayNew(4, sizeof(Item))};
  add_number(array_item2, 1);
  add_number(array_item2, 2);
  add_number(array_item2, 3);
  str = json_stringify(array_item2);
  test_str_eq(str, "[1,2,3]");

  Item obj_item = { .type = item_object, .value.object = CMapNew(str_hash, str_eq) };
  obj_item.value.object->valueReleaser = free_item;
  obj_set(obj_item, "abc", new_number(1));
  obj_set(obj_item, "def", new_number(5));
  str = json_stringify(obj_item);

  // We expect str to match one of these; either one is ok.
  char *ok1 = "{\"abc\":1,\"def\":5}";
  char *ok2 = "{\"def\":5,\"abc\":1}";
  test_that(strcmp(str, ok1) == 0 || strcmp(str, ok2) == 0);

  // Test from items resulting from parsing.
  char *test_data[] = {"1", "null", "true", "false", "[1,2,3]", "{\"a\":3}",
      "[1,{}]", "[\"a\",42,0.5,{\"b\":[]}]"};
  for (int i = 0; i < array_size(test_data); ++i) {
    Item parsed_item;
    json_parse(test_data[i], &parsed_item);
    test_str_eq(json_stringify(parsed_item), test_data[i]);
  }

  // Test that we can parse stringified items.
  Item parsed_item;
  json_parse(json_stringify(array_item1), &parsed_item);
  test_that(parsed_item.type == item_array);

  json_parse(json_stringify(array_item2), &parsed_item);
  test_that(parsed_item.type == item_array);

  json_parse(json_stringify(obj_item), &parsed_item);
  test_that(parsed_item.type == item_object);

  return test_success;
}

int test_unicode_escapes() {
  char u_str[] = {
    0x41, /* A */
    0x5C, /* \ */
    0x22, /* " */
    0xD0, 0x93, /* Cyrillic Ghe */
    0xF0, 0x9D, 0x84, 0x9E, /* Musical symbol G clef */
    0x00 /* terminating null */
  };

  Item item = { .type = item_string, .value.string = u_str };
  char *json = json_stringify(item);
  test_str_eq(json, "\"A\\\\\\\"\\u0413\\uD834\\uDD1E\"");

  Item parsed_item;
  json_parse(json, &parsed_item);
  test_that(parsed_item.type == item_string);
  test_str_eq(parsed_item.value.string, u_str);

  return test_success;
}

int test_parse_tail() {
  char *s = "true false null [1, 2, 3] {}";
  ItemType types[] = { item_true, item_false, item_null, item_array, item_object };
  int i;
  for (i = 0; *s; ++i) {
    test_printf("i=%d *s=0x%02X (%c)\n", i, *s, *s);
    test_that(i < array_size(types));
    Item item;
    s = json_parse(s, &item);
    test_that(item.type == types[i]);
  }
  test_that(i == array_size(types));
  return test_success;
}

int main(int argc, char **argv) {
  start_all_tests(argv[0]);
  run_tests(
    test_parse_number, test_parse_string, test_parse_literals,
    test_parse_arrays, test_parse_objects, test_parse_mixed,
    test_stringify, test_unicode_escapes, test_parse_tail
  );
  return end_all_tests();
}
