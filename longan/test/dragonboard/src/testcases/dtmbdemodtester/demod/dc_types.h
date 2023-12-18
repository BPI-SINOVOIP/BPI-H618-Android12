#ifndef DC_TYPES_H
#define DC_TYPES_H

#define DC_Offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER) 

#define DC_ContainerOf(ptr, type, member) ({ \
    const typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - DC_Offsetof(type,member) ); })

#define DC_NAME_LENGTH 512

#define DC_UNUSE(x) (void)x
#endif
