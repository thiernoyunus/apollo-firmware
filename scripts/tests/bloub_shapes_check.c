/* Checks the ported silhouettes and the two primitives that draw them.
 *
 *   cc -I main/display/bloub -o /tmp/bloub_shapes_check \\
 *      scripts/tests/bloub_shapes_check.c main/display/bloub/bloub_shapes.c -lm
 *
 * No LVGL, no display: a buffer of panel words and the arithmetic. */
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "bloub_shapes.h"

#define W 200
#define H 200
static uint16_t buf[W * H];
static const uint16_t BODY = 0xFFFF, BG = 0x0000;

static int count_body(void) {
    int n = 0;
    for (int i = 0; i < W * H; i++) if (buf[i] == BODY) n++;
    return n;
}

int main(void) {
    /* 1. Every silhouette is a usable profile. */
    for (int s = 0; s < SHAPE_COUNT; s++) {
        float peak = 0.0f;
        for (int i = 0; i < SHAPE_SAMPLES; i++) {
            assert(SHAPE_PROFILES[s][i] > 0.0f);
            if (SHAPE_PROFILES[s][i] > peak) peak = SHAPE_PROFILES[s][i];
        }
        assert(peak > 0.9f && peak < 1.2f);
        assert(shape_name((shape_id_t)s)[0] != 0);
    }

    /* 2. A filled circle is a circle: the right area, and nothing outside it. */
    for (int i = 0; i < W * H; i++) buf[i] = BG;
    bloub_fill_shape(buf, W, H, SHAPE_PROFILES[SHAPE_CIRCLE], 50.0f, 100.0f, 100.0f, BODY);
    const int filled = count_body();
    const double expected = M_PI * 50.0 * 50.0;
    assert(filled > 0);
    assert(fabs(filled - expected) / expected < 0.03);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (buf[y * W + x] != BODY) continue;
            const double dx = x + 0.5 - 100.0, dy = y + 0.5 - 100.0;
            assert(sqrt(dx * dx + dy * dy) <= 50.5);
        }
    }

    /* 3. The eye is erased, and only the eye. */
    bloub_punch_eye(buf, W, H, 100.0f, 100.0f, 20.0f, 20.0f, 0.0f, BG);
    const int after = count_body();
    const int removed = filled - after;
    const double capsule = M_PI * 10.0 * 10.0;   /* a 20x20 capsule is a disc */
    assert(removed > capsule * 0.9 && removed < capsule * 1.1);

    /* 4. Punching the same eye twice changes nothing, and an eye that lands
     *    outside the body erases background rather than body - which is how the
     *    silhouette clips the eye for free. */
    bloub_punch_eye(buf, W, H, 100.0f, 100.0f, 20.0f, 20.0f, 0.0f, BG);
    assert(count_body() == after);
    bloub_punch_eye(buf, W, H, 100.0f, 20.0f, 20.0f, 20.0f, 0.0f, BG);
    assert(count_body() == after);

    /* 5. A shape that is not a circle stays inside its own profile: the
     *    droplet's point is at the top, so no body pixel may appear above it. */
    for (int i = 0; i < W * H; i++) buf[i] = BG;
    bloub_fill_shape(buf, W, H, SHAPE_PROFILES[SHAPE_DROPLET], 60.0f, 100.0f, 100.0f, BODY);
    int top = H;
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            if (buf[y * W + x] == BODY && y < top) top = y;
    assert(top > 100 - 65 && top < 100 - 55);   /* the peak radius, to the pixel */

    printf("ok: %d shapes, circle %d px (%.1f%% of pi r^2), eye removed %d px\n",
           SHAPE_COUNT, filled, 100.0 * filled / expected, removed);
    return 0;
}
