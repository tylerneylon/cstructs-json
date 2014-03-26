// cjson.c
//

#include "cjson.h"

#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


#define CArrayFreeButLeaveElements free

// This compiles as nothing when DEBUG is not defined.
#include "debug_hooks.c"

#define true 1
#define false 0


// Globals.

static char *encoded_chars = "bfnrt\"\\";
static char *decoded_chars = "\b\f\n\r\t\"\\";


// UTF-8 functions.
//
// These are from:
// https://gist.github.com/tylerneylon/9773800

int decode_code_point(char **s) {
  int k = **s ? __builtin_clz(~(**s << 24)) : 0; // Count # of leading 1 bits.
  int mask = (1 << (8 - k)) - 1;  // All 1 bits with k leading 0's.
  int value = **s & mask;
  for (++(*s); k > 0 && **s; --k, ++(*s)) {  // Note that k = #total bytes, or 0.
    value <<= 6;
    value += (**s & 0x3F);
  }
  return value;
}

// Assumes that code is <= 0x10FFFF.
// Ensures that nothing will be written at or beyond end.
void encode_code_point(char **s, char *end, int code) {
  static char val[4];
  int lead_byte_max = 0x7F;
  int val_index = 0;
  while (code > lead_byte_max) {
    val[val_index++] = (code & 0x3F) | 0x80;
    code >>= 6;
    lead_byte_max >>= (val_index == 1 ? 2 : 1);
  }
  val[val_index++] = (code & lead_byte_max) | (~lead_byte_max << 1);
  while (val_index-- && *s < end) {
    **s = val[val_index];
    (*s)++;
  }
}

// Returns 0 if no split was needed.
int split_into_surrogates(int code, int *surr1, int *surr2) {
  if (code <= 0xFFFF) return 0;
  *surr2 = code & 0x3FF;                      // Save the low 10 bits.
  code >>= 10;                                // Drop the low 10 bits.
  *surr1 = code & 0x3F;                       // Save the next 6 bits.
  *surr1 |= ((code & (0x1F << 6)) - 1) << 6;  // Prepend the last 5 bits less 1.
  return 1;
}

// Returns 0 if no join was needed.
int join_from_surrogates(int surr1, int surr2, int *code) {
  if (surr1 < 0xD800 || surr1 > 0xDFFF) return 0;
  *code = ((surr1 & 0x3FF + 0x40) << 10) + (surr2 & 0x3FF);
  return 1;
}


// Internal functions.

int str_hash(void *str_void_ptr) {
  char *str = (char *)str_void_ptr;
  int h = *str;
  while (*str) {
    h *= 84207;
    h += *str++;
  }
  return h;
}

int str_eq(void *str_void_ptr1, void *str_void_ptr2) {
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
  c = *input++; \
  if (c == '\\') { \
    c = *input++; \
    char *esc_ptr = strchr(encoded_chars, c); \
    if (esc_ptr) c = decoded_chars[esc_ptr - encoded_chars]; \
  } \
  *(char *)CArrayNewElement(char_array) = c;

// TODO Rename this; sounds too much like "expression;" it's actually "exponent."
char *parse_exp(Item *item, char *input, char *input_start) {
  if (*input == 'e' || *input == 'E') {
    input++;
    if (*input == '\0') {
      item->type = item_error;
      int index = input - input_start;
      asprintf(&item->value.string, "Error: expected exponent at index %d", index);
      return NULL;
    }
    double exp = 0.0;
    double sign = (*input == '-' ? -1.0 : 1.0);
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
      input++;
    } while (isdigit(*input));
    item->value.number = item->value.number * pow(10.0, exp * sign);
  }
  return input - 1;  // Leave the number pointing at its last character.
}

char *parse_frac(Item *item, char *input, char *input_start) {
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

  // Parse a number.
  int sign = 1;
  if (*input == '-') {
    sign = -1;
    input++;
  }

  if (isdigit(*input)) {
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
    if (input) item->value.number *= sign;
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
    input++;
    item->type = item_string;
    CArray char_array = CArrayNew(16, sizeof(char));
    char c = 1;
    while (c && *input != '"') {
      parse_string_unit(char_array, input);
    }
    if (c == '\0') {
      // We hit the end of the string before we saw a closing quote.
      item->type = item_error;
      asprintf(&item->value.string, "Error: unclosed string");
      CArrayDelete(char_array);
      return NULL;
    }
    *(char *)CArrayNewElement(char_array) = '\0';  // Terminating null.

    item->value.string = char_array->elements;
    CArrayFreeButLeaveElements(char_array);

    return input;
  }

  // Parse an array.
  if (*input == '[') {
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
        subitem->type = item_null;  // It is now safe to release.
        CArrayDelete(array);
        return NULL;
      }
      next_token(input);
    }

    return input;
  }

  // Parse an object.
  if (*input == '{') {
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

      CMapSet(obj, key.value.string, subitem);  // obj takes ownership of both pointers passed in.
      next_token(input);
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

// Expects the input array to have elements of type char *.
static int array_printf(CArray array, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int return_value = vasprintf((char **)CArrayNewElement(array), fmt, args);
  va_end(args);
  return return_value;
}

// Expects the array to have elements of type char *. Caller owns the newly-allocated return val.
static char *array_join(CArray array) {
  int total_len = 0;
  CArrayFor(char **, str_ptr, array) total_len += strlen(*str_ptr);
  char *str = malloc(total_len + 1);  // +1 for the final null.
  char *tail = str;
  CArrayFor(char **, str_ptr, array) tail = stpcpy(tail, *str_ptr);
  return str;
}

#define new_elt(arr) *(char *)CArrayNewElement(arr)

#define add_u_escaped_pt(arr, pt) \
  new_elt(arr) = '\\'; \
  new_elt(arr) = 'u'; \
  for (int i = 0; i < 4; ++i, pt >>= 4) hex_digits[3 - i] = hex[pt % 16]; \
  for (int i = 0; i < 4; ++i) new_elt(arr) = hex_digits[i];

// Caller must free the returned string.
static char *escaped_str(char *s) {
  CArray array = CArrayNew(4, sizeof(char));

  // These are used in add_u_escaped_pt to encode non-ascii unicode characters.
  static char *hex = "0123456789ABCDEF";
  char hex_digits[4];

  while (*s) {
    int code_pt = decode_code_point(&s);
    char *esc_ptr = strchr(decoded_chars, code_pt);
    if (esc_ptr) {
      new_elt(array) = '\\';
      new_elt(array) = encoded_chars[esc_ptr - decoded_chars];
    } else if (code_pt < 0x80) {
      new_elt(array) = code_pt;
    } else {
      int surr1, surr2;
      if (split_into_surrogates(code_pt, &surr1, &surr2)) {
        add_u_escaped_pt(array, surr1);
        add_u_escaped_pt(array, surr2);
      } else {
        add_u_escaped_pt(array, code_pt);
      }
    }
  }
  new_elt(array) = '\0';
  char *escaped_s = array->elements;
  CArrayFreeButLeaveElements(array);
  return escaped_s;
}

void print_item(CArray array, Item item, char *indent, int be_terse) {
  char *next_indent = alloca(strlen(indent) + 1);
  sprintf(next_indent, "%s%s", indent, be_terse ? "" : "  ");  // Nest indents, except when terse.
  char *sep = be_terse ? "," : ",\n";
  char *spc = be_terse ? "" : " ";
  char *lit[] = { [item_true] = "true", [item_false] = "false", [item_null] = "null"};
  int i = 0;  // Used to index obj/arr items in loops within the switch.
  switch (item.type) {
    case item_string:
    case item_error:
      array_printf(array, "\"%s\"", item.value.string);
      break;
    case item_true:
    case item_false:
    case item_null:
      array_printf(array, "%s", lit[item.type]);
      break;
    case item_number:
      array_printf(array, "%g", item.value.number);
      break;
    case item_array:
      array_printf(array, item.value.array->count && !be_terse ? "[\n" : "[");
      CArrayFor(Item *, subitem, item.value.array) {
        array_printf(array, "%s%s", (i++ ? sep : ""), next_indent);
        print_item(array, *subitem, next_indent, be_terse);
      }
      if (item.value.array->count && !be_terse) array_printf(array, "\n%s", indent);
      array_printf(array, "]");
      break;
    case item_object:
      array_printf(array, item.value.object->count && !be_terse ? "{\n" : "{");
      CMapFor(pair, item.value.object) {
        array_printf(array, "%s%s\"%s\":%s", (i++ ? sep : ""), next_indent, (char *)pair->key, spc);
        print_item(array, *(Item *)pair->value, next_indent, be_terse);
      }
      if (item.value.object->count && !be_terse) array_printf(array, "\n%s", indent);
      array_printf(array, "}");
      break;
  }
}

void free_at(void *ptr) {
  free(*(void **)ptr);
}


// Public functions.

Item from_json(char *json_str) {
  Item item;
  char *input = json_str;
  skip_whitespace(input);  // TODO remove macro if we only use it here
  char *tail = parse_value(&item, input, json_str);
  // TODO Check here that the tail is effectively empty.
  return item;
}

char *json_stringify(Item item) {
  CArray str_array = CArrayNew(8, sizeof(char *));
  str_array->releaser = free_at;
  print_item(str_array, item, "" /* indent */, true /* be_terse */);
  //array_printf(str_array, "\n");
  char *json_str = array_join(str_array);
  CArrayDelete(str_array);
  return json_str;
}

void release_item(void *item_ptr) {
  Item *item = (Item *)item_ptr;
  if (item->type == item_string || item->type == item_error) free(item->value.string);
  if (item->type == item_object) CMapDelete(item->value.object);
  if (item->type == item_array) CArrayDelete(item->value.array);
}

void free_item(void *item) {
  release_item(item);
  free(item);
}
