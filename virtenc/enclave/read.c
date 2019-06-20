// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>
#include "syscall.h"

ssize_t ve_read(int fd, void* buf, size_t count)
{
    return ve_syscall3(OE_SYS_read, fd, (long)buf, (long)count);
}