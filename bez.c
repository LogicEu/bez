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

struct Pair {
    size_t data[2];
};

struct Spline {
    vec2 p, c[2];
};

struct Mask {
    struct vector splines;
    int isclosed;
};

struct Roto {
    struct vector masks;
};

const Px red = {255, 0, 0, 255}, green = {0, 255, 0, 255}, blue = {0, 0, 255, 255};

static struct Spline pxSplineCreate(vec2 p)
{
    return (struct Spline) {p, {p, p}};
}

static struct Mask pxMaskCreate(void)
{
    return (struct Mask) {vector_create(sizeof(struct Spline)), 0};
}

static struct Spline* pxMaskPush(struct Mask* mask, vec2 p)
{
    struct Spline spline = pxSplineCreate(p);
    return (struct Spline*)vector_push(&mask->splines, &spline);
}

static void pxMaskFree(struct Mask* mask)
{
    vector_free(&mask->splines);
}

static struct Mask* pxRotoPush(struct Roto* roto)
{
    struct Mask mask = pxMaskCreate();
    return (struct Mask*)vector_push(&roto->masks, &mask);
}

static struct Roto pxRotoCreate(void)
{
    struct Roto roto = {vector_create(sizeof(struct Mask))};
    pxRotoPush(&roto);
    return roto;
}

static void pxRotoFree(struct Roto* roto)
{
    struct Mask* masks = roto->masks.data;
    const size_t count = roto->masks.size;
    for (size_t i = 0; i < count; ++i) {
        pxMaskFree(masks + i);
    }
    vector_free(&roto->masks);
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

static void pxPlotRotoOverlay(const struct Roto* roto, struct Tex2D texture)
{
    const struct Mask* masks = roto->masks.data;
    const size_t mcount = roto->masks.size;
    for (size_t n = 0; n < mcount; ++n) {
        const size_t scount = masks[n].splines.size;
        const struct Spline* splines = masks[n].splines.data;
        for (size_t i = 0; i < scount; ++i) {
            pxPlotLineSmooth(texture, splines[i].p, splines[i].c[0], red);
            pxPlotLineSmooth(texture, splines[i].p, splines[i].c[1], red);
            pxPlot(texture, (int)splines[i].p.x, (int)splines[i].p.y, green);
            pxPlot(texture, (int)splines[i].c[0].x, (int)splines[i].c[0].y, green);
            pxPlot(texture, (int)splines[i].c[1].x, (int)splines[i].c[1].y, green);
        }
    }
}

static struct Pair pxPlotRoto(
    const struct Roto* roto, struct Tex2D texture, const size_t selected, const vec2 m)
{
    struct Pair pair = {selected, 0};
    struct Mask *masks = roto->masks.data;
    const size_t mcount = roto->masks.size;
    const int down = spxeMouseDown(LEFT);
    for (size_t n = 0; n < mcount; ++n) {
        const struct Spline* splines = masks[n].splines.data;
        const size_t scount = masks[n].splines.size;
        if (masks[n].isclosed) {
            pxPlotBezierCubic(
                texture, splines[scount - 1].p, splines[scount - 1].c[1],
                splines[0].c[0], splines[0].p, blue
            );
        }

        for (size_t i = 0; i + 1 < scount; ++i) {
            pxPlotBezierCubic(
                texture, splines[i].p, 
                splines[i].c[1], splines[i + 1].c[0], splines[i + 1].p, blue
            ); 
        }
        
        if (down) {
            const size_t pcount = scount * 3;
            const vec2* points = (vec2*)splines;
            for (size_t i = 0; i < pcount; ++i) {
                if (!pair.data[0] && m.x == points[i].x && m.y == points[i].y) {
                    pair.data[0] = i + 1;
                    pair.data[1] = n + 1;
                    break;
                }
            }
        }
    }

    return pair;
}

int main(const int argc, const char** argv)
{
    int width = 200, height = 150;
    if (argc > 1) {
        width = atoi(argv[1]);
        height = argc > 2 ? atoi(argv[2]) : width;
    }

    struct Roto roto = pxRotoCreate();
    pxRotoPush(&roto);
    Px* fb = spxeStart("bez", 800, 600, width, height);
    Tex2D texture = {fb, width, height};
    const size_t size = width * height * sizeof(Px);
    int isinside = 0, clicked = 0, overlay = 1, justcreated = 0;
    size_t selected = 0, selected_mask = 0;
    ivec2 mouse = {0};
    vec2 m;

    while (spxeRun(texture.pixbuf)) {
        if (spxeKeyPressed(ESCAPE)) {
            break;
        }
        if (spxeKeyPressed(R)) {
            pxRotoFree(&roto);
            pxRotoPush(&roto);
        }
        if (spxeKeyPressed(BACKSPACE)) {
            struct Mask* m = vector_peek(&roto.masks);
            vector_pop(&m->splines);
        }
        if (spxeKeyPressed(X)) {
            struct Mask* m = vector_pop(&roto.masks);
            pxMaskFree(m);
            if (!roto.masks.size) {
                pxRotoPush(&roto);
            }
        }
        if (spxeKeyPressed(Q)) {
            overlay = !overlay;
        }
        if (spxeMouseReleased(LEFT)) {
            selected = 0;
            justcreated = 0;
        }


        spxeMousePos(&mouse.x, &mouse.y);
        m = vec2_from_ivec2(mouse);
        clicked = spxeMousePressed(LEFT);
        isinside = mouse.x >= 0 && mouse.x < width && mouse.y >= 0 && mouse.y < height;
        memset(texture.pixbuf, 155, size);
 
        struct Pair pair = pxPlotRoto(&roto, texture, selected, m);
        if (overlay) {
            pxPlotRotoOverlay(&roto, texture);
        }

        selected = pair.data[0];
        selected_mask = pair.data[1] ? pair.data[1] - 1 : selected_mask;

        if (isinside) {
            struct Mask* mask = vector_index(&roto.masks, selected_mask);
            vec2* points = mask->splines.data;
            if (!mask->isclosed && selected == 1 && mask->splines.size > 2) {
                mask->isclosed = 1;
                pxRotoPush(&roto);
                selected = 0;
                ++selected_mask;
            } else if (selected && points) {
                size_t index = selected - 1 + (!justcreated && spxeKeyDown(Z));
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
                }  else if (spxeKeyDown(LEFT_CONTROL)) {
                    points[index] = m;
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
                    pxMaskPush(vector_peek(&roto.masks), m);
                    justcreated = 1;
                }
                if (spxeMousePressed(RIGHT)) {
                    
                }
            }
        }
    }

    pxRotoFree(&roto);
    return spxeEnd(fb);
}

