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

static struct Spline pxSplineCreate(vec2 p)
{
    return (struct Spline) {p, {p, p}};
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

int main(const int argc, const char** argv)
{
    int width = 200, height = 150;
    if (argc > 1) {
        width = atoi(argv[1]);
        height = argc > 2 ? atoi(argv[2]) : width;
    }

    const Px red = {255, 0, 0, 255}, green = {0, 255, 0, 255}, blue = {0, 0, 255, 255};
    struct vector splinevec = vector_create(sizeof(struct Spline));
    Px* fb = spxeStart("bez", 800, 600, width, height);
    Tex2D texture = {fb, width, height};
    const size_t size = width * height * sizeof(Px);
    int isinside = 0, clicked = 0, down = 0, overlay = 1, isclosed = 0;
    int justcreated = 0;
    size_t i, count, selected = 0;
    ivec2 mouse = {0};
    vec2 m;

    while (spxeRun(texture.pixbuf)) {
        if (spxeKeyPressed(ESCAPE)) {
            break;
        }
        if (spxeKeyPressed(R)) {
            vector_clear(&splinevec);
        }
        if (spxeKeyPressed(X) || spxeKeyPressed(BACKSPACE)) {
            vector_pop(&splinevec);
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
        
        count = splinevec.size;
        vec2* points = splinevec.data;
        struct Spline *splines = splinevec.data;

        if (isclosed) {
            pxPlotBezierCubic(
                texture, splines[count - 1].p,
                splines[count - 1].c[1], splines[0].c[0], splines[0].p, blue
            );
        }

        for (i = 0; i + 1 < count; ++i) {
            pxPlotBezierCubic(
                texture, splines[i].p, 
                splines[i].c[1], splines[i + 1].c[0], splines[i + 1].p, blue
            );

            if (overlay) {
                pxPlotLineSmooth(texture, splines[i].p, splines[i].c[0], red);
                pxPlotLineSmooth(texture, splines[i].p, splines[i].c[1], red);
            }
        }

        if (count && overlay) {
            pxPlotLineSmooth(texture, splines[i].p, splines[i].c[0], red);
            pxPlotLineSmooth(texture, splines[i].p, splines[i].c[1], red);
            count = splinevec.size * 3;
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
            if (!isclosed && selected == 1 && splinevec.size > 2) {
               isclosed = 1;
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
                    vector_push(&splinevec, &s);
                    justcreated = 1;
                }
            }
        }
    }

    vector_free(&splinevec);
    return spxeEnd(fb);
}

