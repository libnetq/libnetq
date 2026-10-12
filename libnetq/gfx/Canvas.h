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

#ifndef NQ_CANVAS_DEFINED_INTERNAL
typedef struct NQCanvas NQCanvas;
typedef struct NQCanvasImage NQCanvasImage;
typedef struct NQCanvasFontFace NQCanvasFontFace;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Both measured away from the baseline, so both are positive. */
typedef struct NQFontMetrics {
  double ascent;
  double descent;
} NQFontMetrics;

NQ_EXPORT const char* NQCanvas_backend(void);

NQ_EXPORT NQCanvas* NQCanvas_create(int width, int height);
NQ_EXPORT NQCanvas* NQCanvas_createForData(void* pixels, int width, int height, int stride);
NQ_EXPORT void NQCanvas_release(NQCanvas*);

NQ_EXPORT void NQCanvas_save(NQCanvas*);
NQ_EXPORT void NQCanvas_restore(NQCanvas*);

NQ_EXPORT void NQCanvas_translate(NQCanvas*, double x, double y);
NQ_EXPORT void NQCanvas_scale(NQCanvas*, double sx, double sy);
NQ_EXPORT void NQCanvas_rotate(NQCanvas*, double angle);

NQ_EXPORT void NQCanvas_transform(NQCanvas*, double a, double b, double c, double d, double e, double f);
NQ_EXPORT void NQCanvas_setTransform(NQCanvas*, double a, double b, double c, double d, double e, double f);
NQ_EXPORT void NQCanvas_getTransform(NQCanvas*, double m[6]);

NQ_EXPORT void NQCanvas_setColor(NQCanvas*, double r, double g, double b, double a);
NQ_EXPORT void NQCanvas_setLineWidth(NQCanvas*, double lineWidth);
NQ_EXPORT void NQCanvas_setLineCap(NQCanvas*, NQLineCap lineCap);
NQ_EXPORT void NQCanvas_setLineJoin(NQCanvas*, NQLineJoin lineJoin);
NQ_EXPORT void NQCanvas_setMiterLimit(NQCanvas*, double miterLimit);
NQ_EXPORT void NQCanvas_setTextAlign(NQCanvas*, NQTextAlign align);
NQ_EXPORT void NQCanvas_setTextBaseline(NQCanvas*, NQTextBaseline baseline);
NQ_EXPORT void NQCanvas_setTextDirection(NQCanvas*, NQTextDirection direction);

NQ_EXPORT void NQCanvas_beginPath(NQCanvas*);
NQ_EXPORT void NQCanvas_moveTo(NQCanvas*, double x, double y);
NQ_EXPORT void NQCanvas_lineTo(NQCanvas*, double x, double y);
NQ_EXPORT void NQCanvas_arc(NQCanvas*, double x, double y, double radius, double startAngle, double endAngle);
NQ_EXPORT void NQCanvas_arcTo(NQCanvas*, double x1, double y1, double x2, double y2, double radius);
NQ_EXPORT void NQCanvas_quadraticCurveTo(NQCanvas*, double cpx, double cpy, double x, double y);
NQ_EXPORT void NQCanvas_bezierCurveTo(NQCanvas*, double cp1x, double cp1y, double cp2x, double cp2y, double x, double y);
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

NQ_EXPORT void NQCanvas_clip(NQCanvas*);
NQ_EXPORT void NQCanvas_clipRect(NQCanvas*, double x, double y, double w, double h);

NQCanvasFontFace* NQCanvasFontFace_createFromFile(NQCanvas* canvas, const char* path, int index);
NQCanvasFontFace* NQCanvasFontFace_createFromFamily(NQCanvas* canvas, const char* family, NQFontSlant slant, NQFontWeight weight);
NQ_EXPORT void NQCanvas_fontFaceRelease(NQCanvas*, NQCanvasFontFace*);

NQ_EXPORT void NQCanvas_setFontFace(NQCanvas*, NQCanvasFontFace* fontFace);
NQ_EXPORT void NQCanvas_setFontSize(NQCanvas*, double fontSize);

NQ_EXPORT void NQCanvas_fontMetrics(NQCanvas*, NQFontMetrics*);
NQ_EXPORT double NQCanvas_textWidth(NQCanvas*, const char* text);
NQ_EXPORT double NQCanvas_textWidth2(NQCanvas*, const char* text, size_t length);

NQ_EXPORT void NQCanvas_strokeText(NQCanvas*, const char* text, double x, double y);
NQ_EXPORT void NQCanvas_fillText(NQCanvas*, const char* text, double x, double y);
NQ_EXPORT void NQCanvas_strokeText2(NQCanvas*, const char* text, size_t length, double x, double y);
NQ_EXPORT void NQCanvas_fillText2(NQCanvas*, const char* text, size_t length, double x, double y);

NQ_EXPORT NQCanvasImage* NQCanvas_imageCreateRGBA(NQCanvas*, const void* pixels, int width, int height, int stride);
NQ_EXPORT void NQCanvas_imageRelease(NQCanvas*, NQCanvasImage*);
NQ_EXPORT int NQCanvas_imageWidth(NQCanvas*, NQCanvasImage*);
NQ_EXPORT int NQCanvas_imageHeight(NQCanvas*, NQCanvasImage*);

NQ_EXPORT void NQCanvas_drawImage(NQCanvas*, NQCanvasImage*, double x, double y);
NQ_EXPORT void NQCanvas_drawImage2(NQCanvas*, NQCanvasImage*, double x, double y, double dw, double dh);
NQ_EXPORT void NQCanvas_drawImage3(NQCanvas*, NQCanvasImage*, double sx, double sy, double sw, double sh, double dx, double dy, double dw, double dh);

NQ_EXPORT void NQCanvas_flush(NQCanvas*);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_GFX_CANVAS_H */
