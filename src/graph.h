#ifndef PX_GRAPH_H
#define PX_GRAPH_H

#include <spxe.h>
#include <spxmath.h>
#include <spxplot.h>
#include <vector.h>

struct Rect {
    ivec2 p, q;
};

struct Node {
    struct Rect rect;
    struct vector inputs;
    struct vector outputs;
    unsigned char selected, hover, outhover, held, outheld;
};

void pxNodesFree(struct vector* nodes);
void pxNodesUpdate(struct vector* nodes, const Tex2D texture, const ivec2 mouse);
void pxNodesPush(struct vector* nodes, const ivec2 p);

#endif /* PX_GRAPH_H */
