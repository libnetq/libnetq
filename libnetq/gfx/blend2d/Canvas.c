/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/gfx/Canvas.h"
#include "libnetq/gfx/blend2d/Canvas.h"

#ifdef CANVAS_BACKEND_BLEND2D

#include <libnetq/Malloc.h>
#include <libnetq/fs/Dir.h>
#include <libnetq/fs/Path.h>
#include <libnetq/string/String.h>

#include <stdlib.h>

/* the HTML canvas default, 10px */
#define DEFAULT_FONT_SIZE 10.0

/* Takes over the image, destroying it if the canvas cannot be made */
static NQCanvas* NQCanvas_fromImage(BLImageCore* image)
{
  NQCanvas* thiz = (NQCanvas*)NQMalloc(sizeof(NQCanvas));
  if (!thiz) {
    bl_image_destroy(image);
    return NULL;
  }
  thiz->image = *image;
  if (bl_context_init_as(&thiz->context, &thiz->image, NULL) != BL_SUCCESS) {
    bl_context_destroy(&thiz->context);
    bl_image_destroy(&thiz->image);
    NQFree(thiz);
    return NULL;
  }
  bl_path_init(&thiz->path);
  bl_font_init(&thiz->font);
  thiz->fontSize = DEFAULT_FONT_SIZE;
  thiz->text = (NQTextState){0};
  return thiz;
}

NQCanvas* NQCanvas_create(int width, int height)
{
  if (width <= 0 || height <= 0)
    return NULL;
  BLImageCore image;
  if (bl_image_init_as(&image, width, height, BL_FORMAT_PRGB32) != BL_SUCCESS) {
    bl_image_destroy(&image);
    return NULL;
  }
  NQCanvas* thiz = NQCanvas_fromImage(&image);
  /* a new image is uninitialized; start transparent, as the other backends do */
  if (thiz)
    bl_context_clear_all(&thiz->context);
  return thiz;
}

/* PRGB32 pixels are native-endian premultiplied 0xAARRGGBB words; the
 * pixels stay the caller's and must outlive the canvas */
NQCanvas* NQCanvas_createForData(void* pixels, int width, int height, int stride)
{
  if (!pixels || width <= 0 || height <= 0)
    return NULL;
  BLImageCore image;
  if (bl_image_init_as_from_data(&image, width, height, BL_FORMAT_PRGB32, pixels, stride, BL_DATA_ACCESS_RW, NULL, NULL) != BL_SUCCESS) {
    bl_image_destroy(&image);
    return NULL;
  }
  return NQCanvas_fromImage(&image);
}

void NQCanvas_release(NQCanvas* thiz)
{
  if (!thiz)
    return;
  bl_context_destroy(&thiz->context); /* before the image it paints into */
  bl_path_destroy(&thiz->path);
  bl_font_destroy(&thiz->font);
  bl_image_destroy(&thiz->image);
  NQFree(thiz);
}

/* Takes over the face, or destroys it when it is empty */
static NQCanvasFontFace* NQCanvasFontFace_fromFace(BLFontFaceCore* face)
{
  NQCanvasFontFace* thiz = (NQCanvasFontFace*)NQMalloc(sizeof(NQCanvasFontFace));
  if (!thiz) {
    bl_font_face_destroy(face);
    return NULL;
  }
  *thiz = *face;
  return thiz;
}

static bool loadFontData(BLFontDataCore* data, const char* path)
{
  bl_font_data_init(data);
  if (bl_font_data_create_from_file(data, path, BL_FILE_READ_MMAP_ENABLED) == BL_SUCCESS)
    return true;
  bl_font_data_destroy(data);
  return false;
}

NQCanvasFontFace* NQCanvasFontFace_createFromFile(NQCanvas* canvas, const char* path, int index)
{
  NQ_UNUSED_PARAM(canvas);
  if (!path || index < 0)
    return NULL;

  BLFontDataCore data;
  if (!loadFontData(&data, path))
    return NULL;
  BLFontFaceCore face;
  bl_font_face_init(&face);
  /* the face keeps its own reference to the data */
  const BLResult result = bl_font_face_create_from_data(&face, &data, (uint32_t)index);
  bl_font_data_destroy(&data);
  if (result != BL_SUCCESS) {
    bl_font_face_destroy(&face);
    return NULL;
  }
  return NQCanvasFontFace_fromFace(&face);
}

/* Blend2D has no system font lookup, so the faces of every font file in the
 * usual font directories go into one font manager, loaded at the first
 * family lookup and kept for the life of the process */
static NQCanvasFontFaceCache systemFonts;
static bool systemFontsLoaded;

static bool isFontFile(const char* name)
{
  const char* ext = NQStrrchr(name, '.');
  return ext && (NQStrcasecmp(ext, ".ttf") == 0 || NQStrcasecmp(ext, ".otf") == 0 ||
                 NQStrcasecmp(ext, ".ttc") == 0 || NQStrcasecmp(ext, ".otc") == 0);
}

static void addFontFile(NQCanvasFontFaceCache* thiz, const char* path)
{
  BLFontDataCore data;
  if (!loadFontData(&data, path))
    return;
  const uint32_t count = bl_font_data_get_face_count(&data);
  for (uint32_t i = 0; i < count; i++) {
    BLFontFaceCore face;
    bl_font_face_init(&face);
    if (bl_font_face_create_from_data(&face, &data, i) == BL_SUCCESS)
      bl_font_manager_add_face(thiz, &face);
    bl_font_face_destroy(&face);
  }
  bl_font_data_destroy(&data);
}

/* font directories nest by vendor and format, but not deeply */
#define FONT_DIR_DEPTH 4

static void addFontDir(NQCanvasFontFaceCache* thiz, const char* dir, int depth)
{
  if (!dir || depth > FONT_DIR_DEPTH)
    return;
  NQDir* entries = NQDir_open(dir);
  if (!entries)
    return;
  char path[1024];
  while (NQDir_next(entries)) {
    const char* name = NQDir_name(entries);
    if (name[0] == '.')
      continue;
    if (NQPathJoin2(dir, name, path, sizeof(path)) >= (int)sizeof(path))
      continue;
    if (NQDir_isDirectory(entries))
      addFontDir(thiz, path, depth + 1);
    else if (NQDir_isFile(entries) && isFontFile(name))
      addFontFile(thiz, path);
  }
  NQDir_close(entries);
}

/* A directory under the one an environment variable names */
static void addFontDirUnder(NQCanvasFontFaceCache* thiz, const char* variable, const char* subdir)
{
  const char* base = getenv(variable);
  char path[1024];
  if (base && base[0] != '\0' && NQPathJoin2(base, subdir, path, sizeof(path)) < (int)sizeof(path))
    addFontDir(thiz, path, 0);
}

void NQCanvasFontFaceCache_loadSystem(NQCanvasFontFaceCache* thiz)
{
#if defined(_WIN32)
  addFontDirUnder(thiz, "WINDIR", "Fonts");
  addFontDirUnder(thiz, "LOCALAPPDATA", "Microsoft\\Windows\\Fonts");
#elif defined(__APPLE__)
  addFontDir(thiz, "/System/Library/Fonts", 0);
  addFontDir(thiz, "/Library/Fonts", 0);
  addFontDirUnder(thiz, "HOME", "Library/Fonts");
#else
  addFontDir(thiz, "/usr/share/fonts", 0);
  addFontDir(thiz, "/usr/local/share/fonts", 0);
  addFontDirUnder(thiz, "HOME", ".local/share/fonts");
  addFontDirUnder(thiz, "HOME", ".fonts");
#endif
}

static void loadSystemFonts(void)
{
  systemFontsLoaded = true;
  bl_font_manager_init_new(&systemFonts);
  NQCanvasFontFaceCache_loadSystem(&systemFonts);
}

NQCanvasFontFace* NQCanvasFontFace_createFromFamily(NQCanvas* canvas, const char* family, NQFontSlant slant, NQFontWeight weight)
{
  NQ_UNUSED_PARAM(canvas);
  if (!family)
    return NULL;
  if (!systemFontsLoaded)
    loadSystemFonts();

  BLFontQueryProperties properties = {0};
  properties.style = slant == NQ_FONT_SLANT_ITALIC ? BL_FONT_STYLE_ITALIC
                   : slant == NQ_FONT_SLANT_OBLIQUE ? BL_FONT_STYLE_OBLIQUE
                   : BL_FONT_STYLE_NORMAL;
  properties.weight = weight == NQ_FONT_WEIGHT_BOLD ? BL_FONT_WEIGHT_BOLD : BL_FONT_WEIGHT_NORMAL;

  BLFontFaceCore face;
  bl_font_face_init(&face);
  if (bl_font_manager_query_face(&systemFonts, family, NQStrlen(family), &properties, &face) != BL_SUCCESS) {
    bl_font_face_destroy(&face);
    return NULL;
  }
  return NQCanvasFontFace_fromFace(&face);
}

#endif /* CANVAS_BACKEND_BLEND2D */
