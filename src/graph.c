#include <graph.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define STEP 0.02F
#define RADIUS 4.0F
#define ROUNDNESS 0.5F

const static Px red = {255, 0, 0, 255};
const static Px blue = {0, 0, 255, 255};
const static Px orange = {255, 155, 0, 255};

static int pxRectInside(struct Rect r, ivec2 p)
{
    return (p.x > r.p.x - r.q.x && p.x < r.p.x + r.q.x && 
            p.y > r.p.y - r.q.y && p.y < r.p.y + r.q.y);
}

static int pxCircleInside(const ivec2 p, const ivec2 q, const int sqr)
{
    const ivec2 v = {p.x - q.x, p.y - q.y};
    return v.x * v.x + v.y * v.y <= sqr;
}

static ivec2 pxNodeInpos(struct Rect r)
{
    return (ivec2){r.p.x, r.p.y + r.q.y};
}

static ivec2 pxNodeOutpos(struct Rect r)
{
    return (ivec2){r.p.x, r.p.y - r.q.y};
}

static struct Node* pxNodeCreate(ivec2 p)
{
    struct Node* node = calloc(sizeof(struct Node), 1);
    node->rect.p = p;
    node->rect.q.x = 60;
    node->rect.q.y = 20;
    node->inputs = vector_create(sizeof(struct Node*));
    node->outputs = vector_create(sizeof(struct Node*));
    return node; 
}

static void pxNodeFree(struct Node* node)
{
    struct Node** inputs = node->inputs.data;
    const size_t icount = node->inputs.size;
    for (size_t i = 0; i < icount; ++i) {
        struct Node** outputs = inputs[i]->outputs.data;
        const size_t ocount = inputs[i]->outputs.size;
        for (size_t j = 0; j < ocount; ++j) {
            if (outputs[j] == node) {
                vector_remove(&inputs[i]->outputs, j);
                break;
            }
        }
    }
 
    struct Node** outputs = node->outputs.data;
    const size_t ocount = node->outputs.size;
    for (size_t i = 0; i < ocount; ++i) {
        struct Node** inputs = outputs[i]->inputs.data;
        const size_t icount = outputs[i]->inputs.size;
        for (size_t j = 0; j < icount; ++j) {
            if (inputs[j] == node) {
                vector_remove(&outputs[i]->inputs, j);
                break;
            }
        }
    }

    vector_free(&node->inputs);
    vector_free(&node->outputs);
    free(node);
}

static void pxNodePush(struct Node* node, struct Node* output)
{
    vector_push(&node->outputs, &output);
    vector_push(&output->inputs, &node);
}

static void pxPlotNodeSpline(const Tex2D texture, const ivec2 p, const ivec2 q, Px color)
{
    vec2 u = vec2_from_ivec2(p), v = vec2_from_ivec2(q);
    u.y -= 40.0F;
    v.y += 40.0F;
    pxPlotBezier3(texture, vec2_from_ivec2(p), u, v, vec2_from_ivec2(q), color);
}

static void pxNodePlot(const struct Node* node, const Tex2D texture, const ivec2 mouse)
{
    const ivec2 p = pxNodeOutpos(node->rect);
    Px outcol = blue, col = !node->selected ? red : orange;
    if (node->hover) {
        col.r -= 100;
    }
    
    if (node->outhover) {
        outcol.r += 100;
        outcol.g += 100;
    }

    pxPlotRectRound(texture, node->rect.p, node->rect.q, ROUNDNESS, col);
    pxPlotCircleSmooth(texture, p, RADIUS, outcol);
    if (node->inputs.size) {
        pxPlotCircleSmooth(texture, pxNodeInpos(node->rect), RADIUS, outcol);
    }
    
    if (node->outheld) {
        pxPlotCircleSmooth(texture, mouse, RADIUS, outcol);
        pxPlotNodeSpline(texture, p, mouse, outcol); 
    }

    struct Node** outputs = node->outputs.data;
    for (size_t i = 0; i < node->outputs.size; ++i) {
        ivec2 q = pxNodeInpos(outputs[i]->rect);
        pxPlotNodeSpline(texture, p, q, outcol);
    }
}

static int pxNodeUpdate(
    struct vector* allnodes, struct Node* node, const ivec2 mouse, const ivec2 dif,
    const int pressed, const int down)
{
    int released = 0;
    const ivec2 p = pxNodeOutpos(node->rect);
    node->outhover = pxCircleInside(p, mouse, RADIUS * RADIUS);
    node->hover = pxRectInside(node->rect, mouse);
    
    if (node->outhover && pressed) {
        node->outheld = 1;
    } else if (node->hover && pressed) {
        node->selected = 1;
        node->held = 1;
    } else if (pressed) {
        node->selected = 0;
    }

    if (!down) {
        released = node->outheld;
        node->outheld = 0;
        node->held = 0; 
    }
 
    if (node->held) {
        node->rect.p.x += dif.x;
        node->rect.p.y += dif.y;
        if (spxeKeyPressed(KEY_BACKSPACE)) {
            size_t search = vector_search(allnodes, &node);
            pxNodeFree(node);
            vector_remove(allnodes, search - 1);
        }
    }

    return released;
}

void pxNodesPush(struct vector* nodes, const ivec2 p)
{
    struct Node* node = pxNodeCreate(p);
    vector_push(nodes, &node);
}

void pxNodesUpdate(
    struct vector* allnodes, const Tex2D texture, const ivec2 mouse)
{
    static ivec2 mouse_node = {0, 0};
    const ivec2 dif = {mouse.x - mouse_node.x, mouse.y - mouse_node.y};
    size_t hover = 0, released = 0;
    const int pressed = spxeMousePressed(MOUSE_LEFT);
    const int down = spxeMouseDown(MOUSE_LEFT);
    const size_t count = allnodes->size;
    struct Node** nodes = allnodes->data;
    for (size_t i = 0; i < count; ++i) {
        if (pxNodeUpdate(allnodes, nodes[i], mouse, dif, pressed, down) && !released) {
            released = i + 1;   
        } else if (!hover && nodes[i]->hover) {
            hover = i + 1;
        }
        pxNodePlot(nodes[i], texture, mouse);
    }

    mouse_node = mouse;
    if (hover && released) {
        if (!vector_search(&nodes[released - 1]->outputs, nodes + hover - 1)) {
            pxNodePush(nodes[released - 1], nodes[hover - 1]);
        }
    }

    if (spxeKeyPressed(KEY_Z)) {
        pxNodesPush(allnodes, mouse);
    } else if (spxeKeyPressed(KEY_X)) {
        struct Node** node = vector_pop(allnodes);
        if (node) {
            pxNodeFree(*node);
        }
    }
}

void pxNodesFree(struct vector* nodes)
{
    struct Node** allnodes = nodes->data;
    const size_t count = nodes->size;
    for (size_t i = 0; i < count; ++i) {
        vector_free(&allnodes[i]->inputs);
        vector_free(&allnodes[i]->outputs);
    }
}

