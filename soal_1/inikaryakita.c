#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>

static const char *dirpath = "/home/rafaelega24/SISOP/modul4/1/portofolio";

static int sisopgampang_getattr(const char *path, struct stat *st) {
    int res;
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);
    res = lstat(fpath, st);
    if (res == -1)
        return -errno;
    return 0;
}

static int sisopgampang_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    char fpath[1000];
    if (strcmp(path, "/") == 0) {
        path = dirpath;
        sprintf(fpath, "%s", path);
    } else {
        sprintf(fpath, "%s%s", dirpath, path);
    }
    int res = 0;
    DIR *dp = opendir(fpath);
    if (dp == NULL)
        return -errno;
    struct dirent *de;
    while ((de = readdir(dp)) != NULL){
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        res = (filler(buf, de->d_name, &st, 0));
        if (res != 0)
            break;
    }
    closedir(dp);
    return 0;
}

static int sisopgampang_mkdir(const char *path, mode_t mode) {
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);
    int res = mkdir(fpath, mode);
    if (res == -1)
        return -errno;
    return 0;
}

static int sisopgampang_rmdir(const char *path) {
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);
    int res = rmdir(fpath);
    if (res == -1)
        return -errno;
    return 0;
}

static int sisopgampang_rename(const char *from, const char *to) {
    char fromPath[1000], toPath[1000];
    sprintf(fromPath, "%s%s", dirpath, from);
    sprintf(toPath, "%s%s", dirpath, to);

    if (strstr(toPath, "/wm") != NULL){
        int srcfd = open(fromPath, O_RDONLY);
        if (srcfd == -1) {
            perror("Error: Cannot open source file for reading.");
            return -errno;
        }
        int destfd = open(toPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (destfd == -1) {
            perror("Error: Cannot open destination file for writing.");
            close(srcfd);
            return -errno;
        }

        char watermark_text[] = "inikaryakita.id";
        char command[1000];
        sprintf(command, "convert -gravity south -geometry +0+20 /proc/%d/fd/%d -fill white -pointsize 36 -annotate +0+0 '%s' /proc/%d/fd/%d", getpid(), srcfd, watermark_text, getpid(), destfd);

        int res = system(command);
        if (res == -1) {
            perror("Error: Failed to execute ImageMagick command.");
            close(srcfd);
            close(destfd);
            return -errno;
        }

        close(srcfd);
        close(destfd);

        if (unlink(fromPath) == -1) {
            perror("Error: Failed to remove the source file.");
            return -errno;
        }
    }else {
        int res = rename(fromPath, toPath);
        if (res == -1) {
            perror("Error: Failed to move the file.");
            return -errno;
        }
    }        
    return 0;
}

static int sisopgampang_chmod(const char *path, mode_t mode) {
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);
    if (strstr(fpath, "/script.sh") != NULL) {
        return -EACCES; // Permission denied
    }

    int res = chmod(fpath, mode);
    if (res == -1)
        return -errno;
    return 0;
}

static int reverse_content(const char *path) {
    int fd = open(path, O_RDWR);
    if (fd == -1) return -errno;

    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return -errno;
    }
    size_t size = st.st_size;
    char *content = malloc(size);
    if (content == NULL) {
        close(fd);
        return -ENOMEM;
    }
    ssize_t bytesRead = pread(fd, content, size, 0);
    if (bytesRead == -1) {
        free(content);
        close(fd);
        return -errno;
    }
    for (size_t i = 0; i < size / 2; i++) {
        char temp = content[i];
        content[i] = content[size - i - 1];
        content[size - i - 1] = temp;
    }
    lseek(fd, 0, SEEK_SET);
    ssize_t bytesWritten = write(fd, content, size);
    if (bytesWritten == -1) {
        free(content);
        close(fd);
        return -errno;
    }
    free(content);
    close(fd);
    return 0;
}

static int sisopgampang_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);
    int fd = creat(fpath, mode);
    if (fd == -1) return -errno;
    fi->fh = fd;

    if (strstr(fpath, "/test") != NULL) {
        close(fd);
        int res = reverse_content(fpath);
        if (res != 0) return res;
        fd = open(fpath, fi->flags);
        if (fd == -1) return -errno;
        fi->fh = fd;
    }
    return 0;
}

static int sisopgampang_rm(const char *path) {
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);
    int res = unlink(fpath);
    if (res == -1)
        return -errno;
    return 0;
}

static int reverse_buffer(char *buf, size_t size) {
    for (size_t i = 0; i < size / 2; i++) {
        char temp = buf[i];
        buf[i] = buf[size - i - 1];
        buf[size - i - 1] = temp;
    }
    return 0;
}

static int sisopgampang_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int fd;
    int res;
    char fpath[1000];

    sprintf(fpath, "%s%s", dirpath, path);
    fd = open(fpath, O_RDONLY);
    if (fd == -1) return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1) {
        close(fd);
        return -errno;
    }
    if (strstr(fpath, "/test") != NULL) {
        reverse_buffer(buf, res);
    }
    close(fd);
    return res;
}

static int sisopgampang_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int fd;
    int res;
    char fpath[1000];

    sprintf(fpath, "%s%s", dirpath, path);
    fd = open(fpath, O_WRONLY);
    if (fd == -1) return -errno;

    if (strstr(fpath, "/test") != NULL) {
        char *rev_buf = malloc(size);
        if (!rev_buf) {
            close(fd);
            return -ENOMEM;
        }
        memcpy(rev_buf, buf, size);
        reverse_buffer(rev_buf, size);
        res = pwrite(fd, rev_buf, size, offset);
        free(rev_buf);
    } else {
        res = pwrite(fd, buf, size, offset);
    }

    if (res == -1) res = -errno;
    close(fd);
    return res;
}

static struct fuse_operations sisopgampang_oper = {
    .getattr = sisopgampang_getattr,
    .readdir = sisopgampang_readdir,
    .mkdir = sisopgampang_mkdir,
    .rmdir = sisopgampang_rmdir,
    .rename = sisopgampang_rename,
    .chmod = sisopgampang_chmod,
    .create = sisopgampang_create,
    .unlink = sisopgampang_rm,
    .read = sisopgampang_read,
    .write = sisopgampang_write,
};

int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &sisopgampang_oper, NULL);
}
