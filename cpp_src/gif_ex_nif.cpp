#include "image_ex_nif.h"

#define MSF_GIF_IMPL
#include "msf_gif.h"

struct resource_t {
  MsfGifState gif_state;
  int width;
  int height;
  int bit_depth;
};

ERL_NIF_TERM create_gif_nif(ErlNifEnv* env, int argc,
                            const ERL_NIF_TERM argv[]) {
  ErlNifResourceType* type = (ErlNifResourceType*)enif_priv_data(env);
  resource_t* resource =
      (resource_t*)enif_alloc_resource(type, sizeof(resource_t));
  new (resource) resource_t();

  if (resource == NULL) {
    return ERROR(env, "Failed to create resource");
  }

  int width;
  if (enif_get_int(env, argv[0], &width) == 0) {
    return ERROR(env, "Invalid width");
  }

  int height;
  if (enif_get_int(env, argv[1], &height) == 0) {
    return ERROR(env, "Invalid height");
  }

  int bit_depth;
  if (enif_get_int(env, argv[2], &bit_depth) == 0) {
    return ERROR(env, "Invalid bit_depth");
  }

  resource->width = width;
  resource->height = height;
  resource->bit_depth = bit_depth;

  msf_gif_begin(&resource->gif_state, width, height);

  ERL_NIF_TERM term = enif_make_resource(env, (void*)resource);
  enif_release_resource(resource);
  return OK(env, term);
}

ERL_NIF_TERM gif_add_frame_nif(ErlNifEnv* env, int argc,
                               const ERL_NIF_TERM argv[]) {
  ErlNifResourceType* type = (ErlNifResourceType*)enif_priv_data(env);
  resource_t* res;
  if (enif_get_resource(env, argv[0], type, (void**)&res) == 0) {
    return ERROR(env, "Invalid handle");
  }

  ErlNifBinary blob;
  if (enif_inspect_binary(env, argv[1], &blob) == 0) {
    return ERROR(env, "Bad argument");
  }

  int delay;
  if (enif_get_int(env, argv[2], &delay) == 0) {
    return ERROR(env, "Invalid delay");
  }

  msf_gif_frame(&res->gif_state, blob.data, delay, res->bit_depth,
                res->width * 4);

  return OK(env, argv[0])
}

ERL_NIF_TERM gif_end(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]) {
  ErlNifResourceType* type = (ErlNifResourceType*)enif_priv_data(env);
  resource_t* res;
  if (enif_get_resource(env, argv[0], type, (void**)&res) == 0) {
    return ERROR(env, "Invalid handle");
  }

  MsfGifResult result = msf_gif_end(&res->gif_state);
  if (!result.data) {
    msf_gif_free(result);
  }

  ERL_NIF_TERM data;
  unsigned char* raw = enif_make_new_binary(env, result.dataSize, &data);
  memcpy(raw, result.data, result.dataSize);

  msf_gif_free(result);

  return OK(env, data);
}

static ErlNifFunc funcs[] = {
    {"create_gif", 3, create_gif_nif, 0},
    {"gif_add_frame", 3, gif_add_frame_nif, 0},
    {"gif_end", 1, gif_end, 0},
};

static void nif_destroy(ErlNifEnv* env, void* obj) {
  resource_t* resource = (resource_t*)obj;
  resource->~resource_t();
}

static int nif_load(ErlNifEnv* env, void** data, const ERL_NIF_TERM info) {
  void* type = enif_open_resource_type(env, "Elixir", "TestEx", nif_destroy,
                                       ERL_NIF_RT_CREATE, NULL);
  if (type == NULL) {
    return -1;
  }

  *data = type;
  return 0;
}

ERL_NIF_INIT(Elixir.ImageEx.Base, funcs, nif_load, NULL, NULL, NULL)
