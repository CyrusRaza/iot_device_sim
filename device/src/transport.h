
//#include "some_header.h"

#ifndef TYPEDEF
#define TYPEDEF
typedef struct trans transport_type;
#endif

typedef struct transport_vtable{
    int (*init)(transport_type *self);
    int (*connect)(transport_type *self);
    int (*poll)(transport_type *self);
    int (*publish)(transport_type *self);
    int (*destroy)(transport_type *self);

}transport_vtable;

typedef struct trans{
    transport_vtable* vt;
}transport_type;