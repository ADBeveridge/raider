#pragma once

int (*unlink_orig)(const char *filename);
int (*lstat_orig)( const char *file_name, struct stat *buf );
int (*fstat_orig)(int filedes, struct stat *buf);
int (*rename_orig)(const char *oldpath, const char *newpath);
int (*open_orig)(const char *filename, int flags);
int (*close_orig)(int fd);
ssize_t (*read_orig)(int fd, void *buf, size_t count);
ssize_t (*write_orig)(int fd, const void *buf, size_t count);
int (*sync_orig)(void);
int (*fsync_orig)(int fd);
off_t (*lseek_orig)(int fildes, off_t offset, int whence);
int (*setrlimit_orig)(int resource, const struct rlimit *rlim);
int (*brk_orig)(void *end_data_segment);
char* (*getcwd_orig)(char *buf, size_t size);
