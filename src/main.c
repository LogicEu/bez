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
#include <roto.h>
#include <graph.h>

int main(const int argc, const char** argv)
{
    const Px red = {255, 0, 0, 255};
    int width = 400, height = 300, mode = 1;
    if (argc > 1) {
        width = atoi(argv[1]);
        height = argc > 2 ? atoi(argv[2]) : width;
    }
 
    Px* pixbuf = spxeStart("nodes", 800, 600, width, height);
    if (!pixbuf) {
        fprintf(stderr, "%s: could not init spxe\n", argv[0]);
        return EXIT_FAILURE;
    }

    Tex2D texture = {pixbuf, width, height};
    struct Roto roto = pxRotoCreate();
    struct vector nodes = vector_create(sizeof(struct Node*));
    const size_t size = width * height * sizeof(Px);
    ivec2 mouse;

    while (spxeRun(texture.pixbuf)) {
        spxeMousePos(&mouse.x, &mouse.y);
        if (spxeKeyPressed(KEY_ESCAPE)) {
            break;
        }
        if (spxeKeyPressed(KEY_SPACE)) {
            mode = !mode;
        }

        memset(texture.pixbuf, 155, size);
        switch (mode) {
        case 0:
            pxRotoUpdate(&roto, texture, vec2_from_ivec2(mouse));
            break;
        case 1:
            pxNodesUpdate(&nodes, texture, mouse);
            break;
        }
        
        if (pxInside(texture, mouse.x, mouse.y)) {
            pxPlot(texture, mouse.x, mouse.y, red);
        }
    }

    pxNodesFree(&nodes);
    pxRotoFree(&roto);
    return spxeEnd(pixbuf);
}

