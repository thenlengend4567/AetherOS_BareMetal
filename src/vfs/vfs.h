#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>

// Struct for virtual file handles tracking base pointer, current offset, and total size
typedef struct vfs_handle {
    const uint8_t *base;   // Pointer to the memory-mapped file base address
    size_t offset;         // Current seek offset from base
    size_t size;           // Total size of the file in bytes
    int is_open;           // State tracker: 1 if open, 0 if closed
} vfs_handle_t;

// Seek whence constants
#define VFS_SEEK_SET 0
#define VFS_SEEK_CUR 1
#define VFS_SEEK_END 2

// Global configuration variables for WAD memory-mapping
extern const uint8_t *g_vfs_wad_base;
extern size_t g_vfs_wad_size;

// VFS Function Declarations
vfs_handle_t *vfs_open(const char *path);
int vfs_close(vfs_handle_t *handle);
size_t vfs_read(vfs_handle_t *handle, void *buffer, size_t bytes);
size_t vfs_write(vfs_handle_t *handle, const void *buffer, size_t bytes);
int vfs_seek(vfs_handle_t *handle, intptr_t offset, int whence);
size_t vfs_tell(const vfs_handle_t *handle);
int vfs_eof(const vfs_handle_t *handle);

#endif // VFS_H
