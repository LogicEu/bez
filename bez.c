#define UTOPIA_IMPLEMENTATION
#define SPXE_APPLICATION
#define SPXP_APPLICATION
#define SPXM_APPLICATION
#include <vector.h>
#include <spxe.h>
#include <spxmath.h>
#include <spxplot.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct Spline {
    vec2 p, c[2];
};

struct Mask {
    struct vector splines;
    int isclosed;
};

static struct Spline pxSplineCreate(vec2 p)
{
    return (struct Spline) {p, {p, p}};
}

static struct Mask pxMaskCreate(void)
{
    return (struct Mask) {vector_create(sizeof(struct Spline)), 0};
}

static void pxPlotBezierCubic(
    const Tex2D texture, vec2 a, vec2 b, vec2 c, vec2 d, const Px col)
{
    float t;
    vec2 q, p = a;
    const float dx = pxAbs(d.x - a.x), dy = pxAbs(d.y - a.y);
    const float dxy = dx + dy;
    const float delta = dxy != 0.0F ? 1.0F / dxy : 1.0F;
    
    for (t = delta; t < 1.0F; t += delta) { 
        float u = 1.0F - t;
        float tt = t * t;
        float uu = u * u;
        float uuu = uu * u;
        float ttt = tt * t;
        float uut = 3.0F * uu * t;
        float utt = 3.0F * u * tt;
        
        q.x = uuu * a.x + uut * b.x + utt * c.x + ttt * d.x;
        q.y = uuu * a.y + uut * b.y + utt * c.y + ttt * d.y;

        pxPlotLine(texture, ivec2_from_vec2(p), ivec2_from_vec2(q), col);
        p = q;
    }
    
    q = d;
    pxPlotLineSmooth(texture, p, q, col);
}

static void pxRotoFree(struct vector* roto)
{
    struct Mask* masks = roto->data;
    const size_t count = roto->size;
    for (size_t i = 0; i < count; ++i) {
        vector_free(&masks[i].splines);
    }
    vector_free(roto);
}

int main(const int argc, const char** argv)
{
    int width = 200, height = 150;
    if (argc > 1) {
        width = atoi(argv[1]);
        height = argc > 2 ? atoi(argv[2]) : width;
    }

    const Px red = {255, 0, 0, 255}, green = {0, 255, 0, 255}, blue = {0, 0, 255, 255};
    struct vector roto = vector_create(sizeof(struct Mask));
    Px* fb = spxeStart("bez", 800, 600, width, height);
    Tex2D texture = {fb, width, height};
    const size_t size = width * height * sizeof(Px);
    int isinside = 0, clicked = 0, down = 0, overlay = 1, justcreated = 0;
    size_t i, count, selected = 0;
    ivec2 mouse = {0};
    vec2 m;

    struct Mask new_mask = pxMaskCreate();
    struct Mask* mask = vector_push(&roto, &new_mask);

    while (spxeRun(texture.pixbuf)) {
        if (spxeKeyPressed(ESCAPE)) {
            break;
        }
        if (spxeKeyPressed(R)) {
            pxRotoFree(&roto);
            new_mask = pxMaskCreate();
            mask = vector_push(&roto, &new_mask);
        }
        if (spxeKeyPressed(BACKSPACE)) {
            vector_pop(&mask->splines);
        }
        if (spxeKeyPressed(Q)) {
            overlay = !overlay;
        }

        spxeMousePos(&mouse.x, &mouse.y);
        m = vec2_from_ivec2(mouse);
        clicked = spxeMousePressed(LEFT);
        down = spxeMouseDown(LEFT);
        isinside = mouse.x >= 0 && mouse.x < width && mouse.y >= 0 && mouse.y < height;
        memset(texture.pixbuf, 155, size);
        
        vec2* points = mask->splines.data;
        struct Spline *splines = mask->splines.data;
        count = mask->splines.size;

        const struct Mask *masks = roto.data;
        for (size_t n = 0; n < roto.size; ++n) {
            const struct Spline* rsplines = masks[n].splines.data;
            const size_t nsplines = masks[n].splines.size;
            if (masks[n].isclosed) {
                pxPlotBezierCubic(
                    texture, rsplines[nsplines - 1].p, rsplines[nsplines - 1].c[1],
                    rsplines[0].c[0], rsplines[0].p, blue
                );
            }

            for (i = 0; i + 1 < masks[n].splines.size; ++i) {
                pxPlotBezierCubic(
                    texture, rsplines[i].p, 
                    rsplines[i].c[1], rsplines[i + 1].c[0], rsplines[i + 1].p, blue
                );

                if (overlay) {
                    pxPlotLineSmooth(texture, rsplines[i].p, rsplines[i].c[0], red);
                    pxPlotLineSmooth(texture, rsplines[i].p, rsplines[i].c[1], red);
                }
            }

            if (nsplines && overlay) {
                pxPlotLineSmooth(texture, rsplines[i].p, rsplines[i].c[0], red);
                pxPlotLineSmooth(texture, rsplines[i].p, rsplines[i].c[1], red);
            }
        }

        if (count && overlay) {
            count = mask->splines.size * 3;
            for (i = 0; i < count; ++i) {
                pxPlot(texture, (int)points[i].x, (int)points[i].y, green);
                if (!selected && down && m.x == points[i].x && m.y == points[i].y) {
                    selected = i + 1;
                }
            }
        }

        if (spxeMouseReleased(LEFT)) {
            selected = 0;
            justcreated = 0;
        }

        if (isinside) {
            if (!mask->isclosed && selected == 1 && mask->splines.size > 2) {
               mask->isclosed = 1;
               new_mask = pxMaskCreate();
               mask = vector_push(&roto, &new_mask);
               selected = 0;
            } else if (selected) {
                const size_t index = selected - 1;
                if (justcreated) {
                    vec2 d = vec2_sub(m, points[index]);
                    points[index + 1] = vec2_sub(points[index], d);
                    points[index + 2] = m;
                } else if (index % 3 == 0) {
                    vec2 d = vec2_sub(m, points[index]);
                    points[index] = m;
                    points[index + 1].x += d.x;
                    points[index + 1].y += d.y;
                    points[index + 2].x += d.x;
                    points[index + 2].y += d.y;
                } else {
                    const int couple = (index + 1) % 3 == 0 ? -1 : 1;
                    const int anchor = couple == 1 ? -1 : -2;
                    vec2 d = vec2_sub(points[index], points[index + anchor]);
                    points[index] = m;
                    points[index + couple] = vec2_sub(points[index + anchor], d);
                }
            } else {
                pxPlot(texture, mouse.x, mouse.y, red);
                if (clicked) {
                    struct Spline s = pxSplineCreate(m);
                    vector_push(&mask->splines, &s);
                    justcreated = 1;
                }
            }
        }
    }

    pxRotoFree(&roto);
    return spxeEnd(fb);
}

