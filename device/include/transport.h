
//#include "some_header.h"

#ifndef TYPEDEFE
#define TYPEDEFE
typedef struct trans transport_type;
#endif




#ifndef SEMA
#define SEMA

#define SEM_OVERWRITE "/sem_overwrite"
#define SEM_UNDERWRITE "/sem_underwrite"

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