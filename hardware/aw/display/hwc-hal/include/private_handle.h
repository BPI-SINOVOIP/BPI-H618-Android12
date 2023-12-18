#ifndef SUNXI_PRIVATE_HANDLE_H
#define SUNXI_PRIVATE_HANDLE_H

#include GPU_PUBLIC_INCLUDE

namespace sunxi {

static inline const private_handle_t* from(buffer_handle_t buf) {
    return (const private_handle_t*)(buf);
}

} // namespace sunxi

#endif
