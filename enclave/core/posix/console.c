// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/corelibc/string.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/device.h>
#include <openenclave/internal/fs.h>
#include "oe_t.h"

#define MAGIC 0x0b292bab

typedef struct _file
{
    struct _oe_device base;
    uint32_t magic;
    int host_fd;
    uint32_t ready_mask;
} file_t;

static file_t* _cast_file(const oe_device_t* device)
{
    file_t* file = (file_t*)device;

    if (file == NULL || file->magic != MAGIC)
        return NULL;

    return file;
}

static int _consolefs_dup(oe_device_t* file_, oe_device_t** new_file_out)
{
    int ret = -1;
    file_t* file = _cast_file(file_);
    int retval = -1;

    if (new_file_out)
        *new_file_out = NULL;

    if (!file || !new_file_out)
    {
        oe_errno = EINVAL;
        goto done;
    }

    /* Ask the host to perform this operation. */
    {
        int err = 0;

        oe_errno = 0;

        if (oe_posix_dup_ocall(&retval, file->host_fd, &err) != OE_OK)
        {
            oe_errno = EINVAL;
            goto done;
        }

        if (retval == -1)
        {
            oe_errno = err;
            goto done;
        }
    }

    /* Allocation and initialize a new file structure. */
    {
        file_t* new_file;

        if (!(new_file = oe_calloc(1, sizeof(file_t))))
        {
            oe_errno = ENOMEM;
            goto done;
        }

        memcpy(new_file, file, sizeof(file_t));
        new_file->host_fd = retval;
        *new_file_out = (oe_device_t*)new_file;
    }

    ret = 0;

done:
    return ret;
}

static int _consolefs_ioctl(
    oe_device_t* file,
    unsigned long request,
    oe_va_list ap)
{
    OE_UNUSED(file);
    OE_UNUSED(request);
    OE_UNUSED(ap);

    oe_errno = ENOTTY;
    return -1;
}

static int _consolefs_fcntl(oe_device_t* file, int cmd, int arg)
{
    OE_UNUSED(file);
    OE_UNUSED(cmd);
    OE_UNUSED(arg);

    oe_errno = ENOTTY;
    return -1;
}

static ssize_t _consolefs_read(oe_device_t* file_, void* buf, size_t count)
{
    ssize_t ret = -1;
    file_t* file = _cast_file(file_);

    oe_errno = 0;

    if (!file)
    {
        oe_errno = EINVAL;
        goto done;
    }

    /* Ask the host to perform this operation. */
    {
        int err = 0;

        if (oe_posix_read_ocall(&ret, file->host_fd, buf, count, &err) != OE_OK)
        {
            oe_errno = EINVAL;
            goto done;
        }

        if (ret == -1)
            err = oe_errno;
    }

done:
    return ret;
}

static ssize_t _consolefs_write(
    oe_device_t* file_,
    const void* buf,
    size_t count)
{
    ssize_t ret = -1;
    file_t* file = _cast_file(file_);

    oe_errno = 0;

    if (!file)
    {
        oe_errno = EINVAL;
        goto done;
    }

    /* Ask the host to perform this operation. */
    {
        int err = 0;

        if (oe_posix_write_ocall(&ret, file->host_fd, buf, count, &err) !=
            OE_OK)
        {
            oe_errno = EINVAL;
            goto done;
        }

        if (ret == -1)
            oe_errno = err;
    }

done:
    return ret;
}

static ssize_t _consolefs_gethostfd(oe_device_t* file_)
{
    ssize_t ret = -1;
    file_t* file = _cast_file(file_);

    if (!file)
    {
        oe_errno = EINVAL;
        goto done;
    }

    ret = file->host_fd;

done:
    return ret;
}

static uint64_t _consolefs_readystate(oe_device_t* file_)
{
    uint64_t ret = (uint64_t)-1;
    file_t* file = _cast_file(file_);

    if (!file)
    {
        oe_errno = EINVAL;
        goto done;
    }

    ret = file->ready_mask;

done:
    return ret;
}

static off_t _consolefs_lseek(oe_device_t* file_, off_t offset, int whence)
{
    off_t ret = -1;
    file_t* file = _cast_file(file_);

    oe_errno = 0;

    if (!file)
    {
        oe_errno = EINVAL;
        goto done;
    }

    /* Ask the host to perform this operation. */
    {
        int err = 0;

        if (oe_posix_lseek_ocall(&ret, file->host_fd, offset, whence, &err) !=
            OE_OK)
        {
            oe_errno = EINVAL;
            goto done;
        }

        if (ret == (off_t)-1)
            oe_errno = err;
    }

done:
    return ret;
}

static int _consolefs_close(oe_device_t* file_)
{
    int ret = -1;
    file_t* file = _cast_file(file_);

    oe_errno = 0;

    if (!file)
    {
        oe_errno = EINVAL;
        goto done;
    }

    /* Ask the host to perform this operation. */
    {
        int err = 0;

        if (oe_posix_close_ocall(&ret, file->host_fd, &err) != OE_OK)
        {
            oe_errno = EINVAL;
            goto done;
        }

        if (ret == -1)
        {
            oe_errno = err;
            goto done;
        }
    }

    switch (file->host_fd)
    {
        case OE_STDIN_FILENO:
        case OE_STDOUT_FILENO:
        case OE_STDERR_FILENO:
        {
            /* Do not free these statically initialized structures. */
            file->host_fd = -1;
            break;
        }
        default:
        {
            /* Free file obtained with dup(). */
            oe_free(file);
        }
    }

done:
    return ret;
}

static oe_fs_ops_t _ops = {
    .base.clone = NULL,
    .base.dup = _consolefs_dup,
    .base.release = NULL,
    .base.shutdown = NULL,
    .base.ioctl = _consolefs_ioctl,
    .base.fcntl = _consolefs_fcntl,
    .mount = NULL,
    .unmount = NULL,
    .open = NULL,
    .base.read = _consolefs_read,
    .base.write = _consolefs_write,
    .base.get_host_fd = _consolefs_gethostfd,
    .base.ready_state = _consolefs_readystate,
    .lseek = _consolefs_lseek,
    .base.close = _consolefs_close,
    .getdents = NULL,
    .stat = NULL,
    .access = NULL,
    .link = NULL,
    .unlink = NULL,
    .rename = NULL,
    .truncate = NULL,
    .mkdir = NULL,
    .rmdir = NULL,
};

static file_t _stdin_file = {
    .base.type = OE_DEVICETYPE_FILESYSTEM,
    .base.size = sizeof(file_t),
    .base.ops.fs = &_ops,
    .magic = MAGIC,
    .host_fd = OE_STDIN_FILENO,
    .ready_mask = 0,
};

static file_t _stdout_file = {
    .base.type = OE_DEVICETYPE_FILESYSTEM,
    .base.size = sizeof(file_t),
    .base.ops.fs = &_ops,
    .magic = MAGIC,
    .host_fd = OE_STDOUT_FILENO,
    .ready_mask = 0,
};

static file_t _stderr_file = {
    .base.type = OE_DEVICETYPE_FILESYSTEM,
    .base.size = sizeof(file_t),
    .base.ops.fs = &_ops,
    .magic = MAGIC,
    .host_fd = OE_STDERR_FILENO,
    .ready_mask = 0,
};

int oe_initialize_console_devices(void)
{
    int ret = -1;

    if (!oe_set_fd_device(OE_STDIN_FILENO, &_stdin_file.base))
        goto done;

    if (!oe_set_fd_device(OE_STDOUT_FILENO, &_stdout_file.base))
        goto done;

    if (!oe_set_fd_device(OE_STDERR_FILENO, &_stderr_file.base))
        goto done;

    ret = 0;

done:

    return ret;
}
