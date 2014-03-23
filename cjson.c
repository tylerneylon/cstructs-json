// cjson.c
//

#include "cjson.h"

#include <stdio.h>
#include <string.h>

// Steps:
// 1. Add error-reporting for arrays.
// 2. Parse objects.
// 3. Parse literals (false/true/null).
// 4. Parse numbers.
// 5. Make items self-cleaning (recursively delete all that's needed on delete).



// Internal functions.

// Using macros is a hacky-but-not-insane (in my opinion)
// way to ensure these 'functions' are inlined.

#define skip_whitespace(input) \
  input += strspn(input, " \t\r\n" );

#define next_token(input) \
  input++; \
  input += strspn(input, " \t\r\n");

#define parse_string_unit(char_array, input) \
  char c = *input++; \
  if (c == '\\') { \
    c = *input++; \
    switch (c) { \
      case 'b': \
        c = '\b'; \
        break; \
      case 'f': \
        c = '\f'; \
        break; \
      case 'n': \
        c = '\n'; \
        break; \
      case 'r': \
        c = '\r'; \
        break; \
      case 't': \
        c = '\t'; \
        break; \
    } \
  } \
  *(char *)CArrayNewElement(char_array) = c;

Item parse_string(char *json_str) {
  return (Item) {0, 0};
}

Item parse_array(char *json_str) {
  return (Item) {0, 0};
}

// At the end, the input points to the last
// character of the parsed value.
char *parse_value(Item *item, char *input, char *input_start) {

  // Skip any leading whitespace.
  skip_whitespace(input);

  // Handle a string.
  if (*input == '"') {
    input++;
    item->type = item_string;
    CArray char_array = CArrayNew(16, sizeof(char));
    while (*input != '"') {
      parse_string_unit(char_array, input);
    }

    item->value.string = char_array->elements;
    free(char_array);  // Leaves the actual char array in memory.

    return input;
  }

  if (*input == '[') {
    next_token(input);
    item->type = item_array;
    CArray array = CArrayNew(8, sizeof(Item));
    while (*input != ']') {
      if (array->count) {
        if (*input != ',') {
          item->type = item_error;
          int index = input - input_start;
          asprintf(&item->value.string, "Error: expected ']' or ',' at index %d", index);
          return NULL;
          // TODO clean up after ourselves
        }
        next_token(input);
      }
      Item *subitem = (Item *)CArrayNewElement(array);
      input = parse_value(subitem, input, input_start);
      if (input == NULL) {
        *item = *subitem;  // TODO Clean up after ourselves.
        return NULL;
      }
      next_token(input);
    }
    item->value.array = array;
    return input;
  }

  // TODO Implement the rest.
  //if (*input == '[') return parse_array(input);
  return NULL;
}


// Private functions.

Item from_json(char *json_str) {
  Item item;
  char *tail = parse_value(&item, json_str, json_str);
  // TODO Check here that the tail is effectively empty.
  return item;
}

char *to_json(Item item) {
  return NULL;
}
