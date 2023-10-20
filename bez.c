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

void pxPlotBezierCubic(const Tex2D texture, vec2 a, vec2 b, vec2 c, vec2 d, const Px col)
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
    int width = 80, height = 60;
    if (argc > 1) {
        width = atoi(argv[1]);
        height = argc > 2 ? atoi(argv[2]) : width;
    }

    const Px red = {255, 0, 0, 255}, green = {0, 255, 0, 255}, blue = {0, 0, 255, 255};
    struct vector point_vector = vector_create(sizeof(vec2));
    Px* fb = spxeStart("bezier", 800, 600, width, height);
    Tex2D texture = {fb, width, height};
    const size_t size = width * height * sizeof(Px);
    int isinside = 0, clicked = 0, down = 0;
    size_t i, count, released, selected = 0;
    ivec2 mouse = {0};
    vec2 m;

    while (spxeRun(texture.pixbuf)) {
        if (spxeKeyPressed(ESCAPE)) {
            break;
        } else if (spxeKeyPressed(R)) {
            vector_clear(&point_vector);
        }

        spxeMousePos(&mouse.x, &mouse.y);
        m = vec2_from_ivec2(mouse);
        clicked = spxeMousePressed(LEFT);
        down = spxeMouseDown(LEFT);
        isinside = mouse.x >= 0 && mouse.x < width && mouse.y >= 0 && mouse.y < height;
        memset(texture.pixbuf, 155, size);

        count = point_vector.size;
        vec2* points = point_vector.data;
        for (i = 0; i + 4 <= count; i += 3) {
            pxPlotBezierCubic(
                texture, points[i], points[i + 1], points[i + 2], points[i + 3], blue
            );

            pxPlotLineSmooth(texture, points[i], points[i + 1], red);
            pxPlotLineSmooth(texture, points[i + 1], points[i + 2], red);
            pxPlotLineSmooth(texture, points[i + 2], points[i + 3], red);
        }

        for (i = 0; i < count; ++i) {
            pxPlot(texture, (int)points[i].x, (int)points[i].y, green);
            if (!selected && down && m.x == points[i].x && m.y == points[i].y) {
                selected = i + 1;
            }
        }
        
        if (spxeMouseReleased(LEFT)) {
            selected = 0;
        }

        if (isinside) {
            if (selected) {
                points[selected - 1] = m;
            } else {
                pxPlot(texture, mouse.x, mouse.y, red);
                if (clicked) {
                    vector_push(&point_vector, &m);
                }
            }
        }
    }

    vector_free(&point_vector);
    return spxeEnd(fb);
}

