#ifdef __cplusplus
extern "C" {
#endif
#include <sys/types.h>

void _init(void) { }
void _fini(void) { }

void* __dso_handle = 0;



extern char _end;  // Defined in linker script

caddr_t _sbrk(int incr) {
    static char *heap_end;
    char *prev_heap_end;

    if (!heap_end) heap_end = &_end;

    prev_heap_end = heap_end;
    heap_end += incr;

    return (caddr_t) prev_heap_end;
}

#ifdef __cplusplus
}
#endif
