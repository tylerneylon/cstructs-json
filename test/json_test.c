// json_test.c
//
// For testing us some cjson.
//

#include "json.h"

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

typedef struct {
  char *str;
  json_Item item;
} StringAndItem;

static void check_parse(StringAndItem str_and_item) {
  test_printf("About to parse:\n%s\n", str_and_item.str);
  json_Item parsed_item;
  json_parse(str_and_item.str, &parsed_item);

  test_printf("Result has raw type %d\n", parsed_item.type);
  test_printf("Result has type %s\n", item_type_names[parsed_item.type]);

  json_Item expected_item = str_and_item.item;
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

  json_Item item, subitem;

  // Non-error cases.
  parse_to_item("[\"abc\", \"def\"]");
  test_that(item.type == item_array);
  test_that(item.value.array->count == 2);
  subitem = CArrayElementOfType(item.value.array, 1, json_Item);
  test_that(strcmp(subitem.value.string, "def") == 0);
  json_release_item(&item);

  parse_to_item(" [ \"abc\", \n \"def\" ] ");
  test_that(item.type == item_array);
  test_that(item.value.array->count == 2);
  subitem = CArrayElementOfType(item.value.array, 1, json_Item);
  test_that(strcmp(subitem.value.string, "def") == 0);
  json_release_item(&item);

  parse_to_item("[]");
  test_that(item.type == item_array);
  test_that(item.value.array->count == 0);
  json_release_item(&item);

  parse_to_item("[[], [[]]]");
  test_that(item.type == item_array);
  test_that(item.value.array->count == 2);
  test_that(CArrayElementOfType(item.value.array, 1, json_Item).type == item_array);
  json_release_item(&item);

  // Error cases.
  char *error_strings[] = {
    "[\"abc\" \"def\"]", "[", "[1", "[1,", "[1,]"
  };
  for (int i = 0; i < array_size(error_strings); ++i) {
    parse_to_item(error_strings[i]);
    test_that(item.type == item_error);
    json_release_item(&item);
  }

  test_that(cjson_net_arr_allocs == 0);

  return test_success;
}

int test_parse_objects() {

  json_Item item, *subitem;

  // Non-error cases.
  parse_to_item("{\"a\": \"def\"}");
  test_that(item.type == item_object);
  CMapFor(pair, item.value.object) {
    test_that(strcmp((char *)pair->key, "a") == 0);
    subitem = (json_Item *)pair->value;
    test_that(subitem->type == item_string);
    test_that(strcmp((char *)subitem->value.string, "def") == 0);
  }
  json_release_item(&item);

  parse_to_item("{ \"str\" : \"ing\" , \"arr\":[\"a\", []]}");
  test_that(item.type == item_object);
  test_that(item.value.object->count == 2);
  test_that(CMapFind(item.value.object, "str") != NULL);
  test_that(CMapFind(item.value.object, "arr") != NULL);
  test_that(CMapFind(item.value.object, "not-a-key") == NULL);

  subitem = (json_Item *)(CMapFind(item.value.object, "str")->value);
  test_that(subitem->type == item_string);
  test_that(strcmp(subitem->value.string, "ing") == 0);
  json_release_item(&item);

  parse_to_item("{}");
  test_that(item.type == item_object);
  json_release_item(&item);

  // Error cases.
  char *error_strings[] = {
    "{", "{, }", "{1: 2}", "{\"a\":}", "{\"b\": 34 \"c\": 35}", "{\"a\" : 1,}", "{\"a\"}"
  };
  for (int i = 0; i < array_size(error_strings); ++i) {
    parse_to_item(error_strings[i]);
    test_that(item.type == item_error);
    json_release_item(&item);
  }

  test_that(cjson_net_obj_allocs == 0);

  return test_success;
}

int test_parse_mixed() {

  json_Item item, *subitem;

  parse_to_item("[true, \"hi\", {\"apple\": true, \"banana\": false, \"robot\": [null]}]");
  test_that(item.type == item_array);
  test_that(item.value.array->count == 3);
  subitem = (json_Item *)CArrayElement(item.value.array, 2);
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
  *(json_Item *)CArrayNewElement(arr_item.value.array) = \
  (json_Item){ .type = item_number, .value.number = num };

#define new_number(n) \
  (item = malloc(sizeof(json_Item)), \
  *item = (json_Item) { .type = item_number, .value.number = n }, \
  item)

// Expects the value to be an json_Item *.
#define obj_set(obj_item, key, val) \
  CMapSet(obj_item.value.object, key, val)

int test_stringify() {

  // Currently, this test leaks some memory.
  // This is not good practice, but it does work as a test.

  char *str;
  json_Item *item;

  // Test from items directly.
  json_Item array_item1 = { .type = item_array, .value.array = CArrayNew(4, sizeof(json_Item))};
  str = json_stringify(array_item1);
  test_str_eq(str, "[]");

  json_Item array_item2 = { .type = item_array, .value.array = CArrayNew(4, sizeof(json_Item))};
  add_number(array_item2, 1);
  add_number(array_item2, 2);
  add_number(array_item2, 3);
  str = json_stringify(array_item2);
  test_str_eq(str, "[1,2,3]");

  json_Item obj_item = { .type = item_object, .value.object = CMapNew(json_str_hash, json_str_eq) };
  obj_item.value.object->valueReleaser = json_free_item;
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
    json_Item parsed_item;
    json_parse(test_data[i], &parsed_item);
    test_str_eq(json_stringify(parsed_item), test_data[i]);
  }

  // Test that we can parse stringified items.
  json_Item parsed_item;
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

  json_Item item = { .type = item_string, .value.string = u_str };
  char *json = json_stringify(item);
  test_str_eq(json, "\"A\\\\\\\"\\u0413\\uD834\\uDD1E\"");

  json_Item parsed_item;
  json_parse(json, &parsed_item);
  test_that(parsed_item.type == item_string);
  test_str_eq(parsed_item.value.string, u_str);

  return test_success;
}

int test_parse_tail() {
  char *s = "true false null [1, 2, 3] {}";
  json_ItemType types[] = { item_true, item_false, item_null, item_array, item_object };
  int i;
  for (i = 0; *s; ++i) {
    test_printf("i=%d *s=0x%02X (%c)\n", i, *s, *s);
    test_that(i < array_size(types));
    json_Item item;
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
