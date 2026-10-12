/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_GFX_CANVAS_H
#define _LIBNETQ_GFX_CANVAS_H

#include <libnetq/gfx/GfxType.h>

#if defined(CANVAS_BACKEND_NANOVG)
# include <nanovg.h>
typedef NVGcontext NQCanvas;
typedef void NQGFont;
#elif defined(CANVAS_BACKEND_PLUTOVG)
# include <plutovg.h>
typedef plutovg_canvas_t NQCanvas;
typedef plutovg_font_face_t NQGFont;
#elif defined(CANVAS_BACKEND_CAIRO)
#include <libnetq/gfx/cairo/Canvas.h>
#elif defined(CANVAS_BACKEND_QUARTZ)
# include <CoreGraphics/CoreGraphics.h>
typedef struct NQCanvas NQCanvas;
/* what a CTFontDescriptorRef points to: a typeface without a size */
typedef struct __CTFontDescriptor NQGFont;
#elif defined(CANVAS_BACKEND_SKIA)
typedef struct NQCanvas NQCanvas;
/* C code sees the SkFont only through pointers; its size is not used, the
 * canvas sets its own */
# ifdef __cplusplus
class SkFont;
typedef SkFont NQGFont;
# else
typedef struct SkFont NQGFont;
# endif
#else
typedef void NQCanvas;
typedef struct NQGFont NQGFont;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Both measured away from the baseline, so both are positive. */
typedef struct NQGFontMetrics {
  double ascent;
  double descent;
} NQGFontMetrics;

NQ_EXPORT const char* NQCanvas_backend(void);

NQCanvas* NQCanvas_create(int width, int height);
NQCanvas* NQCanvas_createForData(void* pixels, int width, int height, int stride);
NQ_EXPORT void NQCanvas_release(NQCanvas*);

#if defined(CANVAS_BACKEND_QUARTZ)
/* Draws into a context someone else made (a view's), retaining it */
NQ_EXPORT NQCanvas* NQCanvas_fromCGContextRef(CGContextRef context);
#endif

#if defined(CANVAS_BACKEND_CAIRO)
/* Draws into a context someone else made (a surface's), referencing it */
NQ_EXPORT NQCanvas* NQCanvas_fromCairo(cairo_t* cr);
#endif

NQ_EXPORT void NQCanvas_save(NQCanvas*);
NQ_EXPORT void NQCanvas_restore(NQCanvas*);
NQ_EXPORT void NQCanvas_clipRect(NQCanvas*, double x, double y, double w, double h);
NQ_EXPORT void NQCanvas_clip(NQCanvas*);

NQ_EXPORT void NQCanvas_translate(NQCanvas*, double x, double y);
NQ_EXPORT void NQCanvas_scale(NQCanvas*, double sx, double sy);

NQ_EXPORT void NQCanvas_setColor(NQCanvas*, double r, double g, double b, double a);
NQ_EXPORT void NQCanvas_setLineWidth(NQCanvas*, double lineWidth);
NQ_EXPORT void NQCanvas_setLineCap(NQCanvas*, NQLineCap lineCap);
NQ_EXPORT void NQCanvas_setLineJoin(NQCanvas*, NQLineJoin lineJoin);
NQ_EXPORT void NQCanvas_setMiterLimit(NQCanvas*, double miterLimit);
void NQCanvas_setTextAlign(NQCanvas*, NQTextAlign align);
void NQCanvas_setTextBaseline(NQCanvas*, NQTextBaseline baseline);
void NQCanvas_setTextDirection(NQCanvas*, NQTextDirection direction);

NQ_EXPORT void NQCanvas_beginPath(NQCanvas*);
NQ_EXPORT void NQCanvas_moveTo(NQCanvas*, double x, double y);
NQ_EXPORT void NQCanvas_lineTo(NQCanvas*, double x, double y);
NQ_EXPORT void NQCanvas_arc(NQCanvas*, double x, double y, double radius, double startAngle, double endAngle);
NQ_EXPORT void NQCanvas_rect(NQCanvas*, double x, double y, double width, double height);
NQ_EXPORT void NQCanvas_roundedRect(NQCanvas*, double x, double y, double w, double h, double r);
NQ_EXPORT void NQCanvas_ellipse(NQCanvas*, double x, double y, double w, double h);
NQ_EXPORT void NQCanvas_closePath(NQCanvas*);
NQ_EXPORT void NQCanvas_stroke(NQCanvas*);
NQ_EXPORT void NQCanvas_fill(NQCanvas*);

NQ_EXPORT void NQCanvas_fillRect(NQCanvas*, double x, double y, double w, double h);
NQ_EXPORT void NQCanvas_strokeRect(NQCanvas*, double x, double y, double w, double h);
NQ_EXPORT void NQCanvas_fillRoundedRect(NQCanvas*, double x, double y, double w, double h, double r);
NQ_EXPORT void NQCanvas_strokeRoundedRect(NQCanvas*, double x, double y, double w, double h, double r);
NQ_EXPORT void NQCanvas_fillEllipse(NQCanvas*, double x, double y, double w, double h);
NQ_EXPORT void NQCanvas_strokeEllipse(NQCanvas*, double x, double y, double w, double h);

NQ_EXPORT void NQCanvas_strokeLine(NQCanvas* thiz, double x0, double y0, double x1, double y1);

NQ_EXPORT void NQCanvas_strokePolyline(NQCanvas*, const double* xy, size_t n);
NQ_EXPORT void NQCanvas_fillPolygon(NQCanvas*, const double* xy, size_t n);
NQ_EXPORT void NQCanvas_strokePolygon(NQCanvas*, const double* xy, size_t n);

NQGFont* NQGFont_createFromFile(NQCanvas* canvas, const char* path, int index);
NQGFont* NQGFont_createFromFamily(NQCanvas* canvas, const char* family, NQFontSlant slant, NQFontWeight weight);
void NQGFont_release(NQGFont*);

void NQCanvas_setFont(NQCanvas*, const NQGFont* font);
NQ_EXPORT void NQCanvas_setFontSize(NQCanvas*, double fontSize);

NQ_EXPORT NQGFontMetrics NQCanvas_fontMetrics(NQCanvas*);
NQ_EXPORT double NQCanvas_textWidth(NQCanvas*, const char* text);
NQ_EXPORT double NQCanvas_textWidth2(NQCanvas*, const char* text, size_t length);

void NQCanvas_drawText(NQCanvas*, const char* text, double x, double baseline);
void NQCanvas_drawText2(NQCanvas*, const char* text, size_t length, double x, double baseline);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_GFX_CANVAS_H */
