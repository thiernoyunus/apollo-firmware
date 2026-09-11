/* Silhouettes and the two primitives the face is drawn with.
 *
 * Ported from bloub (github.com/jeremy-prt/bloub), MIT Licence,
 * Copyright (c) 2026 Jeremy Perret - see LICENSE beside this file.
 *
 * The body is a filled 64-gon, not a per-pixel test against the radial
 * profile. That test is the same picture and about 66k transcendental calls a
 * frame, which is what made the first naive version unusable - the ESP32 port
 * measured it, and this is the shape they landed on too: per row, a handful of
 * edge crossings and one flat span write.
 *
 * The eyes are HOLES, not shapes laid on top. Filling the body first and then
 * erasing the eye interiors means an eye that slides towards the edge clips
 * itself against the silhouette with no clipping code at all - there is simply
 * nothing left to erase outside the body. It is also what bloub's SVG mask
 * does, and why they say their eyes never need cropping.
 *
 * Colours are the panel's own 16-bit words, background included, so nothing
 * here has to know how they were packed. */

#include "bloub_shapes.h"

#include "bloub_math.h"

#include <math.h>
#include <stddef.h>

typedef struct { float x, y; } pt_t;

void bloub_fill_shape(uint16_t* buf, int w, int h, const float* radii, float scale,
                      float cx, float cy, uint16_t color) {
    if (buf == NULL || radii == NULL || w <= 0 || h <= 0) return;

    pt_t pts[SHAPE_SAMPLES];
    for (int i = 0; i < SHAPE_SAMPLES; i++) {
        const float a = (float)i / SHAPE_SAMPLES * BLOUB_TAU;
        pts[i].x = cx + cosf(a) * radii[i] * scale;
        pts[i].y = cy + sinf(a) * radii[i] * scale;
    }

    for (int y = 0; y < h; y++) {
        const float sy = (float)y + 0.5f;
        float xs[SHAPE_SAMPLES];
        int n = 0;
        for (int i = 0; i < SHAPE_SAMPLES; i++) {
            const pt_t a = pts[i], b = pts[(i + 1) % SHAPE_SAMPLES];
            if ((a.y <= sy && b.y > sy) || (b.y <= sy && a.y > sy))
                xs[n++] = a.x + (sy - a.y) / (b.y - a.y) * (b.x - a.x);
        }
        for (int i = 1; i < n; i++) {          /* a handful of crossings */
            const float v = xs[i];
            int j = i - 1;
            while (j >= 0 && xs[j] > v) { xs[j + 1] = xs[j]; j--; }
            xs[j + 1] = v;
        }
        for (int k = 0; k + 1 < n; k += 2) {
            int x0 = (int)ceilf(xs[k] - 0.5f), x1 = (int)floorf(xs[k + 1] - 0.5f);
            if (x0 < 0) x0 = 0;
            if (x1 > w - 1) x1 = w - 1;
            uint16_t* row = buf + (size_t)y * w;
            for (int x = x0; x <= x1; x++) row[x] = color;
        }
    }
}

void bloub_punch_eye(uint16_t* buf, int w, int h, float cx, float cy, float ew, float eh,
                     float tilt_deg, uint16_t background) {
    if (buf == NULL || w <= 0 || h <= 0) return;
    const float hw = ew * 0.5f, hh = eh * 0.5f;
    if (hw <= 0.0f || hh <= 0.0f) return;

    const float rad = tilt_deg * BLOUB_TAU / 360.0f;
    const float c = cosf(rad), s = sinf(rad);
    const float rr = hw < hh ? hw : hh;        /* corner radius: a capsule */
    const float ix = hw - rr, iy = hh - rr;    /* the rectangle inside it */

    /* Only the eye's own bounding box is touched: an eye is a few hundred
     * pixels, the body it sits in a few thousand. */
    const float bx = fabsf(c) * hw + fabsf(s) * hh;
    const float by = fabsf(s) * hw + fabsf(c) * hh;
    int x0 = (int)floorf(cx - bx), x1 = (int)ceilf(cx + bx);
    int y0 = (int)floorf(cy - by), y1 = (int)ceilf(cy + by);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > w - 1) x1 = w - 1;
    if (y1 > h - 1) y1 = h - 1;

    for (int y = y0; y <= y1; y++) {
        uint16_t* row = buf + (size_t)y * w;
        for (int x = x0; x <= x1; x++) {
            const float dx = (float)x + 0.5f - cx, dy = (float)y + 0.5f - cy;
            const float u = fabsf(dx * c + dy * s);
            const float v = fabsf(-dx * s + dy * c);
            if (u <= ix && v <= hh) { row[x] = background; continue; }
            if (v <= iy && u <= hw) { row[x] = background; continue; }
            const float qx = u - ix, qy = v - iy;
            if (qx * qx + qy * qy <= rr * rr) row[x] = background;
        }
    }
}
