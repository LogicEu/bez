#ifndef PX_ROTO_H
#define PX_ROTO_H

#include <vector.h>
#include <spxe.h>
#include <spxmath.h>
#include <spxplot.h>

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

struct Roto pxRotoCreate(void);
void pxRotoFree(struct Roto* roto);
void pxRotoUpdate(struct Roto* roto, const Tex2D texture, const vec2 mouse);

#endif /* PX_ROTO_H */

