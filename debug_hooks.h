// debug_hooks.h
//
// Home repo: https://github.com/tylerneylon/cstructs-json
//
// This file is designed to be #include'd from cjson.c to
// enable memory allocation checks when the DEBUG macro
// is defined.
//
// It's not designed to be compiled directly.
//

#ifdef DEBUG

int cjson_net_obj_allocs = 0;
int cjson_net_arr_allocs = 0;

// Set up the array hooks.

CArray CArrayNew_dbg(int x, size_t y) {
  cjson_net_arr_allocs++;
  return CArrayNew(x, y);
}
#define CArrayNew CArrayNew_dbg

void CArrayDelete_dbg(CArray x) {
  cjson_net_arr_allocs--;
  CArrayDelete(x);
}
#define CArrayDelete CArrayDelete_dbg

#ifdef CArrayFreeButLeaveElements
#undef CArrayFreeButLeaveElements
#endif

void CArrayFreeButLeaveElements(CArray array) {
  cjson_net_arr_allocs--;
  free(array);
}

// Set up the map (object) hooks.

CMap CMapNew_dbg(Hash x, Eq y) {
  cjson_net_obj_allocs++;
  return CMapNew(x, y);
}
#define CMapNew CMapNew_dbg

void CMapDelete_dbg(CMap x) {
  cjson_net_obj_allocs--;
  CMapDelete(x);
}
#define CMapDelete CMapDelete_dbg

#endif
