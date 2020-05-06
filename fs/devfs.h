#include <types.h>
#include <fs.h>

void _devfs_init(void);

int _devfs_write(int fd, const void* buf, uint32 len);
int _devfs_read(int fd, void* buf, uint32 len);
int _devfs_lseek(int fd, int offset, int whence);
int _devfs_open(int chan, int fd, const char* path, int mode);
int _devfs_close(int fd);

