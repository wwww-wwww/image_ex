#include "image_ex_nif.h"

#define MSF_GIF_IMPL
#include "msf_gif.h"

#include "lodepng.h"

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

std::vector<unsigned char> decode_png(ErlNifBinary blob, unsigned& width,
                                      unsigned& height) {
  std::vector<unsigned char> buffer;
  buffer.insert(buffer.end(), blob.data, blob.data + blob.size);

  std::vector<unsigned char> image;
  unsigned error = lodepng::decode(image, width, height, buffer);
  return image;
}

ERL_NIF_TERM png_decode_nif(ErlNifEnv* env, int argc,
                            const ERL_NIF_TERM argv[]) {
  ErlNifBinary blob;
  if (enif_inspect_binary(env, argv[0], &blob) == 0) {
    return ERROR(env, "Bad argument");
  }

  unsigned width, height;
  std::vector<unsigned char> image = decode_png(blob, width, height);

  ERL_NIF_TERM data;
  unsigned char* raw = enif_make_new_binary(env, image.size(), &data);
  memcpy(raw, image.data(), image.size());

  ERL_NIF_TERM map = enif_make_new_map(env);
  MAP(env, map, "image", data);
  MAP(env, map, "xsize", enif_make_uint(env, width));
  MAP(env, map, "ysize", enif_make_uint(env, height));
  MAP(env, map, "bits_per_sample", enif_make_uint(env, 8));
  MAP(env, map, "num_channels", enif_make_uint(env, 4));

  return OK(env, map);
}

ERL_NIF_TERM png_reencode_nif(ErlNifEnv* env, int argc,
                              const ERL_NIF_TERM argv[]) {
  ErlNifBinary blob;
  if (enif_inspect_binary(env, argv[0], &blob) == 0) {
    return ERROR(env, "Bad argument");
  }

  unsigned width, height;
  std::vector<unsigned char> image = decode_png(blob, width, height);

  lodepng::State state;
  state.encoder.filter_palette_zero = 0;
  state.encoder.add_id = false;
  state.encoder.text_compression = 1;
  state.encoder.zlibsettings.nicematch = 258;
  state.encoder.zlibsettings.lazymatching = 1;
  state.encoder.zlibsettings.windowsize = 32768;
  state.encoder.zlibsettings.minmatch = 3;

  std::vector<unsigned char> buffer;
  if (lodepng::encode(buffer, image, width, height, state)) {
    return ERROR(env, "encoding error");
  }

  ERL_NIF_TERM data;
  unsigned char* raw = enif_make_new_binary(env, buffer.size(), &data);
  memcpy(raw, buffer.data(), buffer.size());

  return OK(env, data);
}

ERL_NIF_TERM png_encode_nif(ErlNifEnv* env, int argc,
                            const ERL_NIF_TERM argv[]) {
  ErlNifBinary blob;
  if (enif_inspect_binary(env, argv[0], &blob) == 0) {
    return ERROR(env, "Bad argument");
  }

  std::vector<unsigned char> in_buffer;
  in_buffer.insert(in_buffer.end(), blob.data, blob.data + blob.size);

  int width;
  if (enif_get_int(env, argv[1], &width) == 0) {
    return ERROR(env, "Invalid width");
  }

  int height;
  if (enif_get_int(env, argv[2], &height) == 0) {
    return ERROR(env, "Invalid height");
  }

  std::vector<unsigned char> out_buffer;

  lodepng::State state;
  state.encoder.filter_palette_zero = 0;
  state.encoder.add_id = false;
  state.encoder.text_compression = 1;
  state.encoder.zlibsettings.nicematch = 258;
  state.encoder.zlibsettings.lazymatching = 1;
  state.encoder.zlibsettings.windowsize = 32768;
  state.encoder.zlibsettings.minmatch = 3;

  if (lodepng::encode(out_buffer, in_buffer.data(), width, height, state)) {
    return ERROR(env, "encoding error");
  }

  ERL_NIF_TERM data;
  unsigned char* raw = enif_make_new_binary(env, out_buffer.size(), &data);
  memcpy(raw, out_buffer.data(), out_buffer.size());

  return OK(env, data);
}

ERL_NIF_TERM png_fast_encode_nif(ErlNifEnv* env, int argc,
                                 const ERL_NIF_TERM argv[]) {
  ErlNifBinary blob;
  if (enif_inspect_binary(env, argv[0], &blob) == 0) {
    return ERROR(env, "Bad argument");
  }

  std::vector<unsigned char> in_buffer;
  in_buffer.insert(in_buffer.end(), blob.data, blob.data + blob.size);

  int width;
  if (enif_get_int(env, argv[1], &width) == 0) {
    return ERROR(env, "Invalid width");
  }

  int height;
  if (enif_get_int(env, argv[2], &height) == 0) {
    return ERROR(env, "Invalid height");
  }

  int bit_depth;
  if (enif_get_int(env, argv[3], &bit_depth) == 0) {
    return ERROR(env, "Invalid bit_depth");
  }

  int channels;
  if (enif_get_int(env, argv[4], &channels) == 0) {
    return ERROR(env, "Invalid num_channels");
  }

  std::vector<unsigned char> encoded;
  encoded.resize(FPNGEOutputAllocSize(bit_depth / 8, channels, width, height));

  size_t encoded_size =
      FPNGEEncode(bit_depth / 8, channels, in_buffer.data(), width,
                  width * channels, height, encoded.data());

  ERL_NIF_TERM data;
  unsigned char* raw = enif_make_new_binary(env, encoded_size, &data);
  memcpy(raw, encoded.data(), encoded_size);

  return OK(env, data);
}

static ErlNifFunc funcs[] = {
    {"png_decode", 1, png_decode_nif, ERL_NIF_DIRTY_JOB_CPU_BOUND},
    {"png_encode", 3, png_encode_nif, ERL_NIF_DIRTY_JOB_CPU_BOUND},
    {"png_fast_encode", 5, png_fast_encode_nif, ERL_NIF_DIRTY_JOB_CPU_BOUND},
    {"png_reencode", 1, png_reencode_nif, ERL_NIF_DIRTY_JOB_CPU_BOUND},
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
