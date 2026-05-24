#include "vfs.h"
#include "../libc/freestanding.h"

// Define global configuration variables for the memory-mapped WAD
const uint8_t *g_vfs_wad_base = (const uint8_t *)0x00200000;
size_t g_vfs_wad_size = 8196 * 512; // Configurable size, default ~4.2MB

// Pool of static file handles to avoid dynamic memory allocation (freestanding OS requirement)
#define VFS_MAX_HANDLES 8
static vfs_handle_t vfs_handles[VFS_MAX_HANDLES];

// Helper function to compare strings without standard string.h dependencies
static int vfs_strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

// Open a file mapping "doom1.wad" to memory region at 0x00200000
vfs_handle_t *vfs_open(const char *path) {
    if (!path) {
        return NULL;
    }

    // Only "doom1.wad" is supported in this VFS mapping
    if (vfs_strcmp(path, "doom1.wad") != 0) {
        return NULL;
    }

    // Find an unused handle from the static pool
    for (int i = 0; i < VFS_MAX_HANDLES; i++) {
        if (!vfs_handles[i].is_open) {
            vfs_handles[i].base = g_vfs_wad_base;
            vfs_handles[i].offset = 0;
            vfs_handles[i].size = g_vfs_wad_size;
            vfs_handles[i].is_open = 1;
            return &vfs_handles[i];
        }
    }

    // Return NULL if all handles are exhausted
    return NULL;
}

// Close a virtual file handle
int vfs_close(vfs_handle_t *handle) {
    if (!handle || !handle->is_open) {
        return -1;
    }
    handle->is_open = 0;
    return 0;
}

// Read bytes from the memory-mapped virtual file handle
size_t vfs_read(vfs_handle_t *handle, void *buffer, size_t bytes) {
    if (!handle || !handle->is_open || !buffer) {
        return 0;
    }

    // If offset is already at or past the end of the file, we cannot read
    if (handle->offset >= handle->size) {
        return 0;
    }

    // Calculate maximum bytes we can actually read
    size_t bytes_to_read = bytes;
    if (handle->offset + bytes_to_read > handle->size) {
        bytes_to_read = handle->size - handle->offset;
    }

    // Perform copy using freestanding memcpy
    if (bytes_to_read > 0) {
        memcpy(buffer, handle->base + handle->offset, bytes_to_read);
        handle->offset += bytes_to_read;
    }

    return bytes_to_read;
}

// Write bytes to the memory-mapped virtual file handle
size_t vfs_write(vfs_handle_t *handle, const void *buffer, size_t bytes) {
    if (!handle || !handle->is_open || !buffer) {
        return 0;
    }

    // If offset is already at or past the end of the file, we cannot write
    if (handle->offset >= handle->size) {
        return 0;
    }

    // Calculate maximum bytes we can actually write
    size_t bytes_to_write = bytes;
    if (handle->offset + bytes_to_write > handle->size) {
        bytes_to_write = handle->size - handle->offset;
    }

    // Perform copy to the memory-mapped space using freestanding memcpy
    if (bytes_to_write > 0) {
        // Safe cast to write to the base memory space (if it resides in writeable RAM)
        memcpy((void *)(handle->base + handle->offset), buffer, bytes_to_write);
        handle->offset += bytes_to_write;
    }

    return bytes_to_write;
}

// Seek to a specific offset based on whence direction
int vfs_seek(vfs_handle_t *handle, intptr_t offset, int whence) {
    if (!handle || !handle->is_open) {
        return -1;
    }

    intptr_t new_offset = 0;

    switch (whence) {
        case VFS_SEEK_SET:
            new_offset = offset;
            break;
        case VFS_SEEK_CUR:
            new_offset = (intptr_t)handle->offset + offset;
            break;
        case VFS_SEEK_END:
            new_offset = (intptr_t)handle->size + offset;
            break;
        default:
            return -1; // Invalid whence parameter
    }

    // Bounds checking: offset cannot be negative or larger than file size
    if (new_offset < 0 || new_offset > (intptr_t)handle->size) {
        return -1;
    }

    handle->offset = (size_t)new_offset;
    return 0;
}

// Return the current offset of the virtual file handle
size_t vfs_tell(const vfs_handle_t *handle) {
    if (!handle || !handle->is_open) {
        return 0;
    }
    return handle->offset;
}

// Return 1 if current offset is at or beyond the file size, 0 otherwise
int vfs_eof(const vfs_handle_t *handle) {
    if (!handle || !handle->is_open) {
        return 1;
    }
    return (handle->offset >= handle->size);
}
