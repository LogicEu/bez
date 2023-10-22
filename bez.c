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

struct Roto {
    struct vector masks;
    size_t selected_point, selected_mask;
    int justcreated;
};

const Px red = {255, 0, 0, 255}, green = {0, 255, 0, 255}, blue = {0, 0, 255, 255};

static vec2 vec2_floor(vec2 p)
{
    return (vec2){round(p.x), round(p.y)};
}

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
    struct Roto roto = {vector_create(sizeof(struct Mask)), 0, 0, 0};
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
    const float delta = dxy != 0.0F ? 4.0F / dxy : 1.0F;
    
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

        //pxPlotLine(texture, ivec2_from_vec2(p), ivec2_from_vec2(q), col);
        vec2 n = vec2_floor(p);
        vec2 m = vec2_floor(q);
        pxPlotLineSmooth(texture, n, m, col);
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

static void pxPlotRoto(const struct Roto* roto, struct Tex2D texture)
{
    struct Mask *masks = roto->masks.data;
    const size_t mcount = roto->masks.size;
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
    }
}

static void pxRotoSelect(struct Roto* roto, const vec2 m)
{
    const struct Mask *masks = roto->masks.data;
    const size_t mcount = roto->masks.size;
    for (size_t n = 0; n < mcount && !roto->selected_point; ++n) {
        const vec2* points = masks[n].splines.data;
        const size_t pcount = masks[n].splines.size * 3;
        for (size_t i = 0; i < pcount; ++i) {
            if (!roto->selected_point && m.x == points[i].x && m.y == points[i].y) {
                roto->selected_point = i + 1;
                roto->selected_mask = n;
                break;
            }
        }
    }
}

static void pxRotoInput(struct Roto* roto, const vec2 mouse)
{
    if (spxeKeyPressed(R)) {
        pxRotoFree(roto);
        pxRotoPush(roto);
    }
    if (spxeKeyPressed(BACKSPACE)) {
        struct Mask* m = vector_peek(&roto->masks);
        vector_pop(&m->splines);
    }
    if (spxeKeyPressed(X)) {
        struct Mask* m = vector_pop(&roto->masks);
        pxMaskFree(m);
        if (!roto->masks.size) {
            pxRotoPush(roto);
        }
    }

    if (spxeMouseDown(LEFT)) {
        pxRotoSelect(roto, mouse);
    } else {
        roto->selected_point = 0;
        roto->justcreated = 0;
    }
}

static void pxRotoEdit(struct Roto* roto, const vec2 m)
{
    struct Mask* mask = vector_index(&roto->masks, roto->selected_mask);
    vec2* points = mask->splines.data;
    if (!mask->isclosed && roto->selected_point == 1 && mask->splines.size > 2) {
        mask->isclosed = 1;
        pxRotoPush(roto);
        roto->selected_point = 0;
        roto->selected_mask++;
    } else if (roto->selected_point && points) {
        size_t index = roto->selected_point - 1; 
        index += (index % 3 == 0) && !roto->justcreated && spxeKeyDown(Z);
        if (roto->justcreated) {
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
        if (spxeMousePressed(LEFT)) {
            mask = vector_peek(&roto->masks);
            pxMaskPush(mask, m);
            roto->selected_mask = mask - (struct Mask*)roto->masks.data;
            roto->selected_point = (mask->splines.size - 1) * 3 + 1;
            roto->justcreated = 1;
        }
    }
}

static void pxRotoUpdate(struct Roto* roto, const vec2 mouse)
{
    pxRotoInput(roto, mouse);
    pxRotoEdit(roto, mouse);
}

int main(const int argc, const char** argv)
{
    int width = 200, height = 150, overlay = 1;
    if (argc > 1) {
        width = atoi(argv[1]);
        height = argc > 2 ? atoi(argv[2]) : width;
    }

    struct Roto roto = pxRotoCreate();
    Px* fb = spxeStart("bez", 800, 600, width, height);
    Tex2D texture = {fb, width, height};
    const size_t size = width * height * sizeof(Px);
    ivec2 mouse;

    while (spxeRun(texture.pixbuf)) {
        spxeMousePos(&mouse.x, &mouse.y);
        if (spxeKeyPressed(ESCAPE)) {
            break;
        }
        if (spxeKeyPressed(Q)) {
            overlay = !overlay;
        }

        pxRotoUpdate(&roto, vec2_from_ivec2(mouse));

        memset(texture.pixbuf, 155, size);
        pxPlotRoto(&roto, texture);
        if (overlay) {
            pxPlotRotoOverlay(&roto, texture);
        }
        
        if (pxInside(texture, mouse.x, mouse.y)) {
            pxPlot(texture, mouse.x, mouse.y, red);
        }
    }

    pxRotoFree(&roto);
    return spxeEnd(fb);
}

