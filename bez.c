/* 

Copyright (c) 2023 Eugenio Arteaga A.

Permission is hereby granted, free of charge, to any 
person obtaining a copy of this software and associated 
documentation files (the "Software"), to deal in the 
Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to 
permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice 
shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

************************* bez ***************************/

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
    int justcreated, overlay, isactive;
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

static void pxMaskFree(struct Mask* mask)
{
    vector_free(&mask->splines);
}

static struct Spline* pxMaskPush(struct Mask* mask, vec2 p)
{
    struct Spline spline = pxSplineCreate(p);
    return (struct Spline*)vector_push(&mask->splines, &spline);
}

static struct Roto pxRotoCreate(void)
{
    return (struct Roto){vector_create(sizeof(struct Mask)), 0, 0, 0, 1, 1};
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

static struct Mask* pxRotoPush(struct Roto* roto)
{
    struct Mask mask = pxMaskCreate();
    return (struct Mask*)vector_push(&roto->masks, &mask);
}

static struct Spline* pxRotoPushSpline(struct Roto* roto, vec2 p)
{
    struct Mask* mask = vector_peek(&roto->masks);
    if (!mask || mask->isclosed) {
        mask = pxRotoPush(roto);
    }

    struct Spline* spline = pxMaskPush(mask, p);
    roto->selected_mask = roto->masks.size - 1;
    roto->selected_point = (mask->splines.size - 1) * 3 + 1;
    roto->justcreated = 1;
    return spline;
}

static struct Spline* pxRotoPushSplineNearest(struct Roto* roto, vec2 p)
{
    const size_t mcount = roto->masks.size;
    if (!mcount) {
        return pxRotoPushSpline(roto, p);
    }

    float min = 9999999.9F;
    size_t minspline = 0, minmask = 0;
    struct Spline spline = pxSplineCreate(p);
    struct Mask* masks = roto->masks.data;
    for (size_t n = 0; n < mcount; ++n) {
        const size_t scount = masks[n].splines.size, notclosed = !masks[n].isclosed;
        struct Spline* splines = masks[n].splines.data;
        for (size_t i = 0; i + notclosed < scount; ++i) {
            const size_t j = (i + 1) % scount; 
            float d1 = vec2_sqmag(vec2_sub(splines[i].p, p));
            float d2 = vec2_sqmag(vec2_sub(splines[j].p, p));
            float d = d1 + d2;
            if (d < min) {
                min = d;
                minspline = i;
                minmask = n;
            }
        }
    }

    minspline = (minspline + 1) % masks[minmask].splines.size;
    roto->selected_mask = minmask;
    roto->selected_point = minspline * 3 + 1;
    roto->justcreated = 1;
    return (struct Spline*)vector_push_at(&masks[minmask].splines, &spline, minspline);
}

static void pxPlotRotoOverlay(const struct Roto* roto, const struct Tex2D texture)
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

static void pxPlotRoto(const struct Roto* roto, const struct Tex2D texture)
{
    struct Mask *masks = roto->masks.data;
    const size_t mcount = roto->masks.size;
    for (size_t n = 0; n < mcount; ++n) {
        const struct Spline* splines = masks[n].splines.data;
        const size_t scount = masks[n].splines.size;
        if (masks[n].isclosed) {
            pxPlotBezier3(
                texture, splines[scount - 1].p, splines[scount - 1].c[1],
                splines[0].c[0], splines[0].p, blue
            );
        }

        for (size_t i = 0; i + 1 < scount; ++i) {
            pxPlotBezier3(
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
    if (spxeKeyPressed(Q)) {
        roto->overlay = !roto->overlay;
    }

    if (spxeKeyPressed(BACKSPACE)) {
        struct Mask* m = vector_peek(&roto->masks);
        vector_pop(&m->splines);
    }
    if (spxeKeyPressed(X)) {
        struct Mask* m = vector_pop(&roto->masks);
        size_t scount = m->splines.size;
        pxMaskFree(m);
        if (!scount && roto->masks.size > 1) {
            m = vector_pop(&roto->masks);
            pxMaskFree(m);
        }
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
    const int cntrl = spxeKeyDown(LEFT_CONTROL);
    const int pressed = spxeMousePressed(LEFT);
    struct Mask* mask = vector_index(&roto->masks, roto->selected_mask);
    if (mask && !mask->isclosed && mask->splines.size > 1 && roto->selected_point == 1) {
        mask->isclosed = 1;
        roto->selected_point = 0;
    } else if (mask && mask->splines.size && roto->selected_point) {
        vec2* points = mask->splines.data;
        size_t index = roto->selected_point - 1;
        index += (index % 3 == 0) && !roto->justcreated && spxeKeyDown(Z);
        if (roto->justcreated) {
            vec2 d = vec2_sub(m, points[index]);
            points[index + 1] = vec2_sub(points[index], d);
            points[index + 2] = m;
        } else if (index % 3 == 0) {
            vec2 d = vec2_sub(m, points[index]);
            points[index] = m;
            vec2_add_inline(points[index + 1], d);
            vec2_add_inline(points[index + 2], d);
            if (spxeKeyPressed(C)) {
                vector_remove(&mask->splines, index / 3);
                roto->selected_point = 0;
            }
        }  else if (cntrl) {
            points[index] = m;
        } else {
            const int couple = (index + 1) % 3 == 0 ? -1 : 1;
            const int anchor = couple == 1 ? -1 : -2;
            if (spxeKeyPressed(C)) {
                points[index] = points[index + anchor];
                roto->selected_point = 0;
            } else {
                vec2 d = vec2_sub(points[index], points[index + anchor]);
                points[index] = m;
                points[index + couple] = vec2_sub(points[index + anchor], d);
            }
        }
    } else if (pressed) {
        if (cntrl) {
            pxRotoPushSplineNearest(roto, m);
        } else {
            pxRotoPushSpline(roto, m);
        }
    }
}

static void pxRotoUpdate(struct Roto* roto, const Tex2D texture, const vec2 mouse)
{ 
    if (spxeKeyPressed(D)) {
        roto->isactive = !roto->isactive;
    }

    if (roto->isactive) {
        pxRotoInput(roto, mouse);
        pxRotoEdit(roto, mouse); 
        pxPlotRoto(roto, texture);
        if (roto->overlay) {
            pxPlotRotoOverlay(roto, texture);
        }
    }
}

int main(const int argc, const char** argv)
{
    int width = 200, height = 150;
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

        memset(texture.pixbuf, 155, size);
        pxRotoUpdate(&roto, texture, vec2_from_ivec2(mouse));
        if (pxInside(texture, mouse.x, mouse.y)) {
            pxPlot(texture, mouse.x, mouse.y, red);
        }
    }

    pxRotoFree(&roto);
    return spxeEnd(fb);
}

