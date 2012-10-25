#ifndef gui_HEADER
#define gui_HEADER

#include "rdis.h"

struct _gui {
    struct _rdis * rdis;
};


struct _gui * gui_create (struct _rdis * rdis);
void          gui_delete (struct _gui * gui);

void          gui_rdgwindow  (struct _gui * gui, uint64_t top_index);
void          gui_funcwindow (struct _gui * gui);

#endif