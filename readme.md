# cstructs-json
*A small json library for use with cstructs.*

This C library parses and stringifies json.
It works with utf-8 encoded strings.

Good things about this library:

* Easy to learn and use
* Efficient
* Resulting objects and arrays are mutable
* Resulting objects have expected constant-time lookups

Bad things about this library:

* Caused slight unibrow growth in some test subjects

## Parse example

```
json_Item item;
json_parse("[1, 2, 3]", &item);
CArrayFor(json_Item *, subitem, item.value.array, index) {
  printf("%g ", subitem->value.number);
}
// Prints out 1 2 3
```

## Stringify example

```
// Set up an array with elements 1, "cat", true.
CArray array = CArrayNew(3, sizeof(json_Item));
#define new_elt *(json_Item *)CArrayNewElement(array) = (json_Item)
new_elt { .type = item_number, .value.number = 1 };
new_elt { .type = item_string, .value.string = "cat" };
new_elt { .type = item_true };
json_Item array_item = { .type = item_array, .value.array = array };

// Turn this array into a json string.
char *str = json_stringify(array_item);

// str is now [1,"cat",true]
```

## Pretty print example

```
char *s = "[1, true, {\"dog\": null}, [], false]";
json_Item item;
json_parse(s, &item);
printf("%s\n", json_pretty_stringify(item));
```

The result of the above is:
```
[
  1,
  true,
  {
    "dog": null
  },
  [],
  false
]
```

Note that parsing and then pretty-printing a string may alter the
order of the keys in objects. This can happen since parsed objects
are represented as hash tables without any idea of a key ordering.

## Documentation

### `char *json_parse(char *json_str, json_Item *item)`

This function accepts a utf-8 encoded string `json_str` which
it parses into `*item`.
On success, the return value is the first unparsed character in `json_str`
after the first valid json value in the string, and after any
trailing whitespace of that json value.

If there is a parse error, `NULL` is returned, and the type
of `*item` is set to `item_error`. You can determine the precise parse
error and location by printing out `item->value.string`.

Any returned item, including error items, can be released by calling
`json_release_item(item)`. This includes recursive memory freeing for
arrays and objects.

If you have a series of whitespace-separated json values in `json_str`,
they can be parsed one after the other by the following loop:

```
char *s = json_str;
do {
  json_Item item;
  s = json_parse(s, &item);
  /* process item; I recommend checking for errors here */
} while (*s);
```

The parser is aware of unicode surrogate pairs, and converts them
to the appropriate code points, which are encoded in standard
(surrogate-pair-free) utf-8 in the output item.

### `char *json_stringify(json_Item item)`

This produces a json string based on the given item.
The produced json string uses no whitespace outside of string values.

If the item includes unicode code points that cannot be represented
in 16-bits - i.e., in json's `\u`-escaped notation - then those code
points are broken up into surrogate pairs in the `\u`-escaped notation.
(This is as per the json spec, but I suspect some stringifiers may skimp on
  this detail.)

### `char *json_pretty_stringify(json_Item item)`

This function is identical to `json_stringify` except that its output
is intended to be consumed by humans. The produced string includes
whitespace that visually clarifies the nesting structure of the item.
See the pretty print example above.
