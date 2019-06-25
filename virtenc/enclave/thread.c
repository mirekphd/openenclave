// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "thread.h"
#include "hexdump.h"
#include "print.h"
#include "syscall.h"

void ve_dump_thread(thread_t* thread)
{
    if (!thread)
        return;

    if (!thread->tls)
        return;

    ve_print("=== DUMP THREAD (%p):\n", thread);

    ve_put("TLS:\n");
    ve_hexdump(thread->tls, thread->tls_size);

    ve_put("THREAD:\n");
    ve_hexdump(thread, sizeof(thread_t));
}

#define VE_ARCH_GET_FS 0x1003

thread_t* ve_thread_self(void)
{
    thread_t* thread = NULL;
    ve_syscall2(OE_SYS_arch_prctl, VE_ARCH_GET_FS, (long)&thread);
    return thread;
}