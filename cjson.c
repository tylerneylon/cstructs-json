// cjson.c
//

#include "cjson.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>


// Internal functions.

static int str_hash(void *str_void_ptr) {
  char *str = (char *)str_void_ptr;
  int h = *str;
  while (*str) {
    h *= 84207;
    h += *str++;
  }
  return h;
}

static int str_eq(void *str_void_ptr1, void *str_void_ptr2) {
  return !strcmp(str_void_ptr1, str_void_ptr2);
}


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

char *parse_exp(Item *item, char *input, char *input_start) {
  printf("%s\n", __func__);
  if (*input == 'e' || *input == 'E') {
    input++;
    printf("Just after e/E, I'm looking at character 0x%02X (%c)\n", *input, *input);
    if (*input == '\0') {
      printf("Error case.\n");
      item->type = item_error;
      int index = input - input_start;
      asprintf(&item->value.string, "Error: expected exponent at index %d", index);
      return NULL;
    }
    double exp = 0.0;
    double sign = (*input == '-' ? -1.0 : 1.0);
    printf("sign set to %f\n", sign);
    if (*input == '-' || *input == '+') input++;
    if (!isdigit(*input)) {
      item->type = item_error;
      int index = input - input_start;
      asprintf(&item->value.string, "Error: expected digit at index %d", index);
      return NULL;
    }
    do {
      exp *= 10.0;
      exp += (*input - '0');
      printf("exp is at %f\n", exp);
      input++;
    } while (isdigit(*input));
    item->value.number = item->value.number * pow(10.0, exp * sign);
    printf("End of %s; *input=%c\n", __func__, *input);
  }
  return input - 1;  // Leave the number pointing at its last character.
}

char *parse_frac(Item *item, char *input, char *input_start) {
  printf("%s\n", __func__);
  if (*input == '.') {
    input++;
    double w = 0.1;
    if (!isdigit(*input)) {
      item->type = item_error;
      int index = input - input_start;
      asprintf(&item->value.string, "Error: expected digit after . at index %d", index);
      return NULL;
    }
    do {
      item->value.number += w * (*input - '0');
      printf("At digit %c, just added quantity %f\n", *input, (w * (*input - '0')));
      w *= 0.1;
      input++;
    } while (isdigit(*input));
  }
  return parse_exp(item, input, input_start);
}

// Assumes there's no leading whitespace.
// At the end, the input points to the last
// character of the parsed value.
char *parse_value(Item *item, char *input, char *input_start) {

  printf("parse_value called at index %ld\n", input - input_start);

  // Parse a number.
  int sign = 1;
  if (*input == '-') {
    sign = -1;
    input++;
  }

  if (isdigit(*input)) {
    printf("starting number at index %ld (excludes leading - if there is one)\n", input - input_start);
    item->type = item_number;
    item->value.number = 0.0;

    if (*input != '0') {
      do {
        item->value.number *= 10.0;
        item->value.number += (*input - '0');
        input++;
      } while (isdigit(*input));
    } else {
      input++;
    }
    input = parse_frac(item, input, input_start);
    if (input) {
      item->value.number *= sign;
      printf("About to leave parse_value after parsing a number with input set to %c\n", *input);
    }
    return input;

  } else if (sign == -1) {

    // A '-' without a number following it.
    item->type = item_error;
    int index = input - input_start;
    asprintf(&item->value.string, "Error: expected digit at index %d", index);
    return NULL;
  }

  // Parse a string.
  if (*input == '"') {
    printf("starting string at index %ld\n", input - input_start);
    input++;
    item->type = item_string;
    CArray char_array = CArrayNew(16, sizeof(char));
    while (*input != '"') {
      parse_string_unit(char_array, input);
    }
    *(char *)CArrayNewElement(char_array) = '\0';  // Terminating null.

    item->value.string = char_array->elements;
    free(char_array);  // Leaves the actual char array in memory.

    return input;
  }

  // Parse an array.
  if (*input == '[') {
    printf("starting array at index %ld\n", input - input_start);
    next_token(input);

    CArray array = CArrayNew(8, sizeof(Item));
    array->releaser = release_item;
    item->type = item_array;
    item->value.array = array;

    while (*input != ']') {
      if (array->count) {
        if (*input != ',') {
          item->type = item_error;
          int index = input - input_start;
          asprintf(&item->value.string, "Error: expected ']' or ',' at index %d", index);
          CArrayDelete(array);
          return NULL;
        }
        next_token(input);
      }
      Item *subitem = (Item *)CArrayNewElement(array);
      input = parse_value(subitem, input, input_start);
      if (input == NULL) {
        *item = *subitem;
        CArrayDelete(array);
        return NULL;
      }
      next_token(input);
    }

    return input;
  }

  // Parse an object.
  if (*input == '{') {
    printf("starting object at index %ld\n", input - input_start);
    next_token(input);
    CMap obj = CMapNew(str_hash, str_eq);
    obj->keyReleaser = free;
    obj->valueReleaser = free_item;
    item->type = item_object;
    item->value.object = obj;
    while (*input != '}') {
      if (obj->count) {
        if (*input != ',') {
          item->type = item_error;
          int index = input - input_start;
          asprintf(&item->value.string, "Error: expected '}' or ',' at index %d", index);
          CMapDelete(obj);
          return NULL;
        }
        next_token(input);
      }

      // Parse the key, which should be a string.
      if (*input != '"') {
        item->type = item_error;
        int index = input - input_start;
        asprintf(&item->value.string, "Error: expected '\"' at index %d", index);
        CMapDelete(obj);
        return NULL;
      }
      Item key;
      input = parse_value(&key, input, input_start);
      if (input == NULL) {
        *item = key;
        CMapDelete(obj);
        return NULL;
      }

      // Parse the separating colon.
      next_token(input);
      //printf("Expecting a colon at index %ld\n", input - input_start);
      if (*input != ':') {
        item->type = item_error;
        int index = input - input_start;
        asprintf(&item->value.string, "Error: expected ':' at index %d", index);
        CMapDelete(obj);
        return NULL;
      }

      // Parse the value of this key.
      next_token(input);
      Item *subitem = (Item *)malloc(sizeof(Item));
      input = parse_value(subitem, input, input_start);
      if (input == NULL) {
        *item = *subitem;
        free(subitem);
        CMapDelete(obj);
        return NULL;
      }

      // obj takes ownership of both pointers passed in.
      CMapSet(obj, key.value.string, subitem);

      next_token(input);
      //printf("At end of obj parse cycle, index=%ld; char=0x%02X (%c)\n", input - input_start, *input, isprint(*input) ? *input : '?');
    }
    return input;
  }

  // Parse a literal: true, false, or null.
  char *literals[3] = {"false", "true", "null"};
  size_t lit_len[3] = {5, 4, 4};
  ItemType types[3] = {item_false, item_true, item_null};

  for (int i = 0; i < 3; ++i) {
    if (*input != literals[i][0]) continue;
    if (strncmp(input, literals[i], lit_len[i]) != 0) {
      item->type = item_error;
      int index = input - input_start;
      asprintf(&item->value.string, "Error: expected '%s' at index %d", literals[i], index);
      return NULL;
    }
    item->type = types[i];
    item->value.bool = (i < 2) ? i : 0;  // The 0 literal is for the null case.
    return input + (lit_len[i] - 1);
  }

  // If we get here, the string is not well-formed.
  item->type = item_error;
  int index = input - input_start;
  asprintf(&item->value.string, "Error: unexpected character (0x%02X) at index %d", *input, index);
  return NULL;
}


// Private functions.

Item from_json(char *json_str) {
  Item item;
  char *input = json_str;
  skip_whitespace(input);  // TODO remove macro if we only use it here
  char *tail = parse_value(&item, input, json_str);
  // TODO Check here that the tail is effectively empty.
  return item;
}

char *to_json(Item item) {
  return NULL;
}

void release_item(void *item_ptr) {
  Item *item = (Item *)item_ptr;
  switch (item->type) {
    case item_string:
    case item_error:
      free(item->value.string);
      break;
    case item_object:
      CMapDelete(item->value.object);
      break;
    case item_array:
      CArrayDelete(item->value.array);
      break;
    default:  // No work needed for direct values.
      break;
  }
}

void free_item(void *item) {
  release_item(item);
  free(item);
}
