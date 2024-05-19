#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <libgen.h>

#define MAX_CHUNK_SIZE 10240

static const char *source_path = "/home/rafaelega24/SISOP/modul4/3/relics";
static char full_path[1024];

typedef struct CombinedFile {
    char path[1024];
    struct CombinedFile *next;
} CombinedFile;

CombinedFile *combined_files_head = NULL;

static void simpanGabungan(const char *path) {
    CombinedFile *new_file = (CombinedFile *)malloc(sizeof(CombinedFile));
    strcpy(new_file->path, path);
    new_file->next = combined_files_head;
    combined_files_head = new_file;
}

static void hapusGabungan() {
    CombinedFile *current = combined_files_head;
    while (current) {
        unlink(current->path);
        CombinedFile *next = current->next;
        free(current);
        current = next;
    }
    combined_files_head = NULL;
}

static int relic_getattr(const char *path, struct stat *statbuf) {
    memset(statbuf, 0, sizeof(struct stat));
    sprintf(full_path, "%s%s", source_path, path);
    if (lstat(full_path, statbuf) == -1) {
        // Check if it's a combined file
        sprintf(full_path, "/tmp/tempe%s", path);
        if (lstat(full_path, statbuf) == -1)
            return -errno;
    }
    return 0;
}

static int relic_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    DIR *dp;
    struct dirent *de;
    char filename[256];
    FILE *combined_file;
    char combined_path[1024];

    (void)offset;
    (void)fi;

    sprintf(full_path, "%s%s", source_path, path);
    dp = opendir(full_path);
    if (dp == NULL)
        return -errno;

    // Add "." and ".." to the directory listing
    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;

        // Check if the entry is a file chunk
        if (de->d_type == DT_REG && strstr(de->d_name, ".") != NULL && strlen(de->d_name) > 4) {
            char *file_ext = strrchr(de->d_name, '.');
            if (file_ext != NULL && strcmp(file_ext, ".000") == 0) {
                // This is the first chunk of a file, combine all chunks
                strcpy(filename, de->d_name);
                filename[strlen(filename) - 4] = '\0'; // Remove the ".000" extension
                sprintf(combined_path, "/tmp/tempe%s/%s", path, filename); // Create combined file in /tmp/tempe

                // Create parent directory if it doesn't exist
                char parent_dir[1024];
                snprintf(parent_dir, sizeof(parent_dir), "/tmp/tempe%s", path);
                mkdir(parent_dir, 0755);

                combined_file = fopen(combined_path, "wb");
                if (combined_file == NULL)
                    return -errno;

                // Append all chunks to the combined file
                int chunk_idx = 0;
                char chunk_path[1024];
                FILE *chunk_file;
                while (1) {
                    sprintf(chunk_path, "%s%s/%s.%03d", source_path, path, filename, chunk_idx);
                    chunk_file = fopen(chunk_path, "rb");
                    if (chunk_file == NULL)
                        break;

                    char buffer[MAX_CHUNK_SIZE];
                    size_t bytes_read;
                    while ((bytes_read = fread(buffer, 1, MAX_CHUNK_SIZE, chunk_file)) > 0) {
                        fwrite(buffer, 1, bytes_read, combined_file);
                    }
                    fclose(chunk_file);
                    chunk_idx++;
                }
                fclose(combined_file);

                // Add the combined file to the FUSE directory listing
                if (filler(buffer, filename, &st, 0))
                    break;

                // Track the combined file
                simpanGabungan(combined_path);
            }
        } else if (de->d_type == DT_DIR) {
            // Add non-file entries (directories) to the listing
            if (filler(buffer, de->d_name, &st, 0))
                break;
        }
    }

    closedir(dp);
    return 0;
}

static int relic_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
    char combined_path[1024];
    int fd;
    int res;

    sprintf(combined_path, "/tmp/tempe%s", path); // Read from /tmp/tempe directory
    fd = open(combined_path, O_RDONLY);
    if (fd == -1)
        return -errno;

    res = pread(fd, buffer, size, offset);
    if (res == -1)
        res = -errno;

    close(fd);
    return res;
}

static int relic_open(const char *path, struct fuse_file_info *fi) {
    return 0;
}

static int relic_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
    char combined_path[1024];
    sprintf(combined_path, "/tmp/tempe%s", path);

    // Ensure parent directory exists
    char parent_dir[1024];
    snprintf(parent_dir, sizeof(parent_dir), "/tmp/tempe%s", path);
    dirname(parent_dir);
    mkdir(parent_dir, 0755);

    int fd = open(combined_path, O_WRONLY | O_CREAT, 0644);
    if (fd == -1)
        return -errno;

    int res = pwrite(fd, buffer, size, offset);
    if (res == -1) {
        close(fd);
        return -errno;
    }
    close(fd);

    // Split the combined file into chunks and write to the source path
    FILE *combined_file = fopen(combined_path, "rb");
    if (combined_file == NULL)
        return -errno;

    FILE *source_file = NULL;
    int chunk_idx = 0;
    char chunk_path[1024];
    char chunk_buffer[MAX_CHUNK_SIZE];
    size_t bytes_read;

    while ((bytes_read = fread(chunk_buffer, 1, MAX_CHUNK_SIZE, combined_file)) > 0) {
        sprintf(chunk_path, "%s%s.%03d", source_path, path, chunk_idx);
        source_file = fopen(chunk_path, "wb");
        if (source_file == NULL) {
            fclose(combined_file);
            return -errno;
        }

        fwrite(chunk_buffer, 1, bytes_read, source_file);
        fclose(source_file);
        chunk_idx++;
    }

    fclose(combined_file);
    return res;
}

static int relic_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    char combined_path[1024];
    sprintf(combined_path, "/tmp/tempe%s", path);

    // Ensure parent directory exists
    char parent_dir[1024];
    snprintf(parent_dir, sizeof(parent_dir), "/tmp/tempe%s", path);
    dirname(parent_dir);
    mkdir(parent_dir, 0755);

    int fd = open(combined_path, O_WRONLY | O_CREAT, mode);
    if (fd == -1)
        return -errno;
    close(fd);
    return 0;
}

static int relic_mkdir(const char *path, mode_t mode) {
    char combined_path[1024];
    sprintf(combined_path, "/tmp/tempe%s", path);

    // Ensure parent directory exists
    char parent_dir[1024];
    snprintf(parent_dir, sizeof(parent_dir), "/tmp/tempe%s", path);
    dirname(parent_dir);
    mkdir(parent_dir, 0755);

    int res = mkdir(combined_path, mode);
    if (res == -1)
        return -errno;
    return 0;
}

static int relic_rmdir(const char *path) {
    char combined_path[1024];
    sprintf(combined_path, "/tmp/tempe%s", path);

    int res = rmdir(combined_path);
    if (res == -1)
        return -errno;
    return 0;
}

static int relic_unlink(const char *path) {
    char chunk_path[2048];
    int chunk_idx = 0;

    // Generate the full path to the file in the source directory
    sprintf(full_path, "%s%s", source_path, path);

    // Loop through and delete all chunks
    while (1) {
        sprintf(chunk_path, "%s.%03d", full_path, chunk_idx);
        int res = unlink(chunk_path);
        if (res == -1) {
            if (errno == ENOENT) {
                break; // No more chunks found
            } else {
                return -errno; // Some other error occurred
            }
        }
        chunk_idx++;
    }

    return 0;
}

static struct fuse_operations relic_oper = {
    .getattr = relic_getattr,
    .readdir = relic_readdir,
    .open = relic_open,
    .read = relic_read,
    .write = relic_write,
    .mkdir = relic_mkdir,
    .rmdir = relic_rmdir,
    .create = relic_create,
    .unlink = relic_unlink,
};

int main(int argc, char *argv[]) {
    umask(0);
    int ret = fuse_main(argc, argv, &relic_oper, NULL);
    hapusGabungan();
    return ret;
}
