#ifdef SIO_SYS_EPOLL
#include "sio_epoll.c"
#else
#include "sio_select.c"
#endif
