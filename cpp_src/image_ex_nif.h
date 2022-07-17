#include "erl_nif.h"
#include "fpnge.h"

#include <vector>
#include <cstring>

ERL_NIF_TERM make_utf8str(ErlNifEnv* env, const char* data) {
  ErlNifBinary utf8;

  size_t datalen = data == NULL ? 0 : strlen(data);
  if (0 == enif_alloc_binary(datalen, &utf8)) {
    return enif_make_badarg(env);
  }

  memcpy(utf8.data, data, datalen);
  return enif_make_binary(env, &utf8);
}

#define ERROR(e, m) \
  enif_make_tuple2(e, enif_make_atom(env, "error"), make_utf8str(env, m));

#define OK(e, m) enif_make_tuple2(e, enif_make_atom(env, "ok"), m);

#define MAP(env, map, key, value) \
  enif_make_map_put(env, map, enif_make_atom(env, key), value, &map);
