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
#include "libnetq/gfx/quartz/Canvas.h"

#ifdef CANVAS_BACKEND_QUARTZ

#include <libnetq/Malloc.h>
#include <libnetq/string/String.h>

NQCanvas* NQCanvas_fromCGContextRef(CGContextRef context)
{
  if (!context)
    return NULL;
  NQCanvas* thiz = (NQCanvas*)NQMalloc(sizeof(NQCanvas));
  if (!thiz)
    return NULL;
  thiz->context = CGContextRetain(context);
  thiz->fontDesc = NULL;
  thiz->font = NULL;
  thiz->fontSize = 12;
  return thiz;
}

static void forgetFont(NQCanvas* thiz)
{
  if (thiz->font)
    CFRelease(thiz->font);
  thiz->font = NULL;
}

/* Drops the canvas's reference; the context is destroyed with the last one. */
void NQCanvas_release(NQCanvas* thiz)
{
  forgetFont(thiz);
  if (thiz->fontDesc)
    CFRelease(thiz->fontDesc);
  NQCanvasContext_release(thiz->context);
  NQFree(thiz);
}

NQGFont* NQGFont_createFromFamily(NQCanvas* canvas, const char* family, NQFontSlant slant, NQFontWeight weight)
{
  (void)canvas;
  if (!family)
    return NULL;
  CFStringRef name = CFStringCreateWithCString(NULL, family, kCFStringEncodingUTF8);
  if (!name)
    return NULL;

  /* CoreText tells italic and oblique faces apart by neither */
  uint32_t traits = 0;
  if (weight == NQ_FONT_WEIGHT_BOLD)
    traits |= kCTFontBoldTrait;
  if (slant != NQ_FONT_SLANT_NORMAL)
    traits |= kCTFontItalicTrait;
  CFNumberRef symbolic = CFNumberCreate(NULL, kCFNumberSInt32Type, &traits);
  const void* traitKeys[] = {kCTFontSymbolicTrait};
  const void* traitValues[] = {symbolic};
  CFDictionaryRef traitDict = CFDictionaryCreate(NULL, traitKeys, traitValues, 1,
                                                 &kCFTypeDictionaryKeyCallBacks,
                                                 &kCFTypeDictionaryValueCallBacks);

  const void* keys[] = {kCTFontFamilyNameAttribute, kCTFontTraitsAttribute};
  const void* values[] = {name, traitDict};
  CFDictionaryRef attrs = CFDictionaryCreate(NULL, keys, values, 2,
                                             &kCFTypeDictionaryKeyCallBacks,
                                             &kCFTypeDictionaryValueCallBacks);
  CTFontDescriptorRef desc = CTFontDescriptorCreateWithAttributes(attrs);

  CFRelease(attrs);
  CFRelease(traitDict);
  CFRelease(symbolic);
  CFRelease(name);
  return (NQGFont*)desc;
}

void NQGFont_release(NQGFont* thiz)
{
  CFRelease(thiz);
}

static CTFontRef drawingFont(NQCanvas* thiz)
{
  if (!thiz->font && thiz->fontDesc)
    thiz->font = CTFontCreateWithFontDescriptor(thiz->fontDesc, thiz->fontSize, NULL);
  return thiz->font;
}

/* Builds a CoreText line of `length` bytes of `text` that takes its color
 * from the context's fill color, so text is drawn in the color set with
 * NQCanvas_setColor. */
static CTLineRef makeLine(const char* text, size_t length, CTFontRef font)
{
  CFStringRef str = CFStringCreateWithBytes(NULL, (const UInt8*)text, (CFIndex)length, kCFStringEncodingUTF8, false);
  if (!str) {
    /* not valid UTF-8; Latin-1 accepts any byte sequence */
    str = CFStringCreateWithBytes(NULL, (const UInt8*)text, (CFIndex)length, kCFStringEncodingISOLatin1, false);
  }
  if (!str)
    return NULL;

  const void* keys[] = {kCTFontAttributeName, kCTForegroundColorFromContextAttributeName};
  const void* values[] = {font, kCFBooleanTrue};
  CFDictionaryRef attrs = CFDictionaryCreate(NULL, keys, values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  CFAttributedStringRef astr = CFAttributedStringCreate(NULL, str, attrs);
  CTLineRef line = CTLineCreateWithAttributedString(astr);

  CFRelease(astr);
  CFRelease(attrs);
  CFRelease(str);
  return line;
}

void NQCanvas_setFont(NQCanvas* thiz, const NQGFont* font)
{
  if (!font || font == thiz->fontDesc)
    return;
  CFRetain(font);
  if (thiz->fontDesc)
    CFRelease(thiz->fontDesc);
  thiz->fontDesc = font;
  forgetFont(thiz);
}

void NQCanvas_setFontSize(NQCanvas* thiz, double size)
{
  if ((CGFloat)size == thiz->fontSize)
    return;
  thiz->fontSize = (CGFloat)size;
  forgetFont(thiz);
}

NQGFontMetrics NQCanvas_fontMetrics(NQCanvas* thiz)
{
  CTFontRef font = drawingFont(thiz);
  if (!font)
    return (NQGFontMetrics){0, 0};

  NQGFontMetrics result;
  result.ascent = CTFontGetAscent(font);
  result.descent = CTFontGetDescent(font);

  return result;
}

double NQCanvas_textWidth(NQCanvas* thiz, const char* text)
{
  return NQCanvas_textWidth2(thiz, text, strlen(text));
}

double NQCanvas_textWidth2(NQCanvas* thiz, const char* text, size_t length)
{
  CTFontRef font = drawingFont(thiz);
  if (!font || length == 0)
    return 0;
  CTLineRef line = makeLine(text, length, font);
  if (!line)
    return 0;
  double w = CTLineGetTypographicBounds(line, NULL, NULL, NULL);
  CFRelease(line);
  return w;
}

void NQCanvas_drawText2(NQCanvas* thiz, const char* text, size_t length, double x, double baseline)
{
  CTFontRef font = drawingFont(thiz);
  if (!font || length == 0)
    return;
  CTLineRef line = makeLine(text, length, font);
  if (!line)
    return;

  /* the context is flipped, so flip glyphs back upright */
  CGContextSetTextMatrix(thiz->context->context, CGAffineTransformMakeScale(1.0, -1.0));
  CGContextSetTextPosition(thiz->context->context, x, baseline);
  CTLineDraw(line, thiz->context);
  CFRelease(line);
}

void NQCanvas_drawText(NQCanvas* thiz, const char* text, double x, double baseline)
{
  NQCanvas_drawText2(thiz, text, strlen(text), x, baseline);
}

#endif /* CANVAS_BACKEND_QUARTZ */
