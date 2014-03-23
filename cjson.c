// cjson.c
//

#include "cjson.h"

#include <string.h>

// Internal functions.

// Using macros is a hacky-but-not-insane (in my opinion)
// way to ensure these 'functions' are inlined.

#define skip_whitespace(input) \
  input += strspn(input, " \t\r\n" );


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

char *parse_string_unit_old(CArray output, char *input) {
  char c = *input++;
  if (c == '\\') {
    c = *input++;
    switch (c) {
      case 'b':
        c = '\b';
        break;
      case 'f':
        c = '\f';
        break;
      case 'n':
        c = '\n';
        break;
      case 'r':
        c = '\r';
        break;
      case 't':
        c = '\t';
        break;
    }
  }
  *(char *)CArrayNewElement(output) = c;
  return input;
}

Item parse_string(char *json_str) {
  return (Item) {0, 0};
}

Item parse_array(char *json_str) {
  return (Item) {0, 0};
}

char *parse_value(Item *item, char *input) {

  // Skip any leading whitespace.
  skip_whitespace(input);

  // Handle a string.
  if (*input == '"') {
    input++;
    item->type = item_string;
    CArray char_array = CArrayNew(16, sizeof(char));
    while (*input != '"') {
      parse_string_unit(char_array, input);
      //input = parse_string_unit(chars, input);
    }
    input++;  // Skip over the ending quote.

    item->value.string = char_array->elements;
    free(char_array);  // Leaves the actual char array in memory.

    return input;
  }

  if (*input == '[') {
    input++;
    skip_whitespace(input);
    item->type = item_array;
    CArray array = CArrayNew(8, sizeof(Item));
    while (*input != ']') {
      if (array->count) {
        if (*input != ',') {
          // TODO report a parse error
        }
        input++;
        skip_whitespace(input);
      }
      input = parse_value((Item *)CArrayNewElement(array), input);
      skip_whitespace(input);
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
  char *tail = parse_value(&item, json_str);
  // TODO Check here that the tail is effectively empty.
  return item;
}

char *to_json(Item item) {
  return NULL;
}
