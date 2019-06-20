// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_SBRK_H
#define _VE_SBRK_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

void* ve_sbrk(intptr_t increment);

#endif /* _VE_SBRK_H */