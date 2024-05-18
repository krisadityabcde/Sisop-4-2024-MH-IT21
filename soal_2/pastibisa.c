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
#include <time.h>
#include <ctype.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

static const char *dirpath = "/home/rafaelega24/SISOP/modul4/2/sensitif";
int password_entered;

// Fungsi untuk mencatat aktivitas ke log
static void buatLog(const char *activity, const char *path, const char *result) {
    FILE *log_file = fopen("/home/rafaelega24/SISOP/modul4/2/logs-fuse.log", "a");
    if (log_file == NULL) {
        perror("Error opening log file");
        return;
    }

    // Mendapatkan waktu sekarang
    time_t now;
    time(&now);
    struct tm *local_time = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%d/%m/%Y-%H:%M:%S", local_time);

    // Menulis ke log file
    fprintf(log_file, "[%s]::%s::[%s]::[%s]\n", result, timestamp, activity, path);

    fclose(log_file);
}

char secret_password[100] = "inipassword";

// Fungsi untuk mengecek dan mengijinkan akses ke folder "rahasia"
int bolehin_gak_ya(const char *path) {
    if (!password_entered) {
            char input_password[100];
            printf("Masukkan kata sandi: ");
            scanf("%s", input_password);
            if (strcmp(input_password, secret_password) != 0) {
                printf("Kata sandi salah. Akses ditolak.\n");
                buatLog("DENIED ACCESS", path, "Akses folder rahasia ditolak");
                return 0;
            }
            password_entered = 1;
            buatLog("GRANTED ACCESS", path, "Akses folder rahasia diterima");
        }
        return 1;
}

// Implementasi eazy getattr untuk mendapatkan atribut file/direktori
static int getattr_eazy(const char *path, struct stat *stbuf) {
    int res;
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);
    res = lstat(fpath, stbuf);
    if (res == -1)
        return -errno;
    return 0;
    }

// Implementasi eazy readdir untuk membaca isi direktori
static int readdir_eazy(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
int res;
char fpath[1000];
    if (strcmp(path, "/") == 0) {
        path = dirpath;
        sprintf(fpath, "%s", path);
    } else {
        sprintf(fpath, "%s%s", dirpath, path);
    }

    if(strstr(fpath, "rahasia") != NULL){
        if (!bolehin_gak_ya(path)) {
            return -EACCES;
        }
    }

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
    password_entered = 0;
    return 0;
}

// Implementasi eazy open untuk membuka file
static int open_eazy(const char *path, struct fuse_file_info *fi) {
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);
    if(strstr(fpath, "rahasia") != NULL){
        if (!bolehin_gak_ya(path)) {
            return -EACCES;
        }
    }
    int res = open(fpath, fi->flags);
    if (res == -1) return -errno;
    close(res);
    return 0;
}

// Implementasi eazy mkdir untuk membuat direktori
static int mkdir_eazy(const char *path, mode_t mode) {
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);
    int res = mkdir(fpath, mode);
    if (res == -1) return -errno;
    buatLog("MKDIR", path, "Berhasil membuat direktori");
    return 0;
}

// Implementasi eazy rmdir untuk menghapus direktori
static int rmdir_eazy(const char *path) {
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);
    int res = rmdir(fpath);
    if (res == -1) return -errno;
    buatLog("RMDIR", path, "Berhasil menghapus direktori");
    return 0;
}

// Implementasi eazy rename untuk mengganti nama file/direktori
static int rename_eazy(const char *from, const char *to) {
    char fromPath[1000], toPath[1000];
    sprintf(fromPath, "%s%s", dirpath, from);
    sprintf(toPath, "%s%s", dirpath, to);
    int res = rename(fromPath, toPath);
    if (res == -1) return -errno;
    buatLog("RENAME/MOVE", from, "Berhasil mengganti nama/memindahkan file");
    return 0;
}

// Implementasi eazy create untuk membuat file
static int create_eazy(const char *path, mode_t mode, struct fuse_file_info *fi) {
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);
    int res = creat(fpath, mode);
    if (res == -1) return -errno;
    close(res);
    buatLog("CREATE", path, "Berhasil membuat file");
    return 0;
}

// Implementasi eazy unlink untuk menghapus file
static int rm_eazy(const char *path) {
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);
    int res = unlink(fpath);
    if (res == -1) return -errno;
    buatLog("REMOVE", path, "Berhasil menghapus file");
    return 0;
}

// Implementasi eazy chmod untuk mengubah mode akses file/direktori
static int chmod_eazy(const char *path, mode_t mode) {
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);
    int res = chmod(fpath, mode);
    if (res == -1) return -errno;
    buatLog("CHMOD", path, "Berhasil mengubah mode akses file/direktori");
    return 0;
}

static void decrypt_file_content(const char *path, char *buf, size_t size) {
    char *filename = strrchr(path, '/');
    if (filename != NULL) {
        filename++;
        if (strstr(filename, "base64") != NULL) {
            BIO *bio, *b64;
            bio = BIO_new(BIO_s_mem());
            b64 = BIO_new(BIO_f_base64());
            BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
            BIO_push(b64, bio);

            // Tulis teks yang akan didekripsi ke dalam BIO
            BIO_write(bio, buf, strlen(buf));

            // Buat buffer untuk teks hasil dekripsi
            char *decoded_text = (char *)malloc(strlen(buf));
            memset(decoded_text, 0, strlen(buf));

            // Lakukan dekripsi dan simpan hasilnya ke dalam buffer
            BIO_read(b64, decoded_text, strlen(buf));

            // Bersihkan BIO dan tutup
            BIO_free_all(b64);
            if (decoded_text != NULL) {
                strncpy(buf, decoded_text, size);
                free(decoded_text);
            }
        } else if (strstr(filename, "rot13") != NULL ) {
            // Dekripsi ROT13
            for (int i = 0; i < size; i++) {
                if (isalpha(buf[i])) {
                    if (islower(buf[i])) {
                        buf[i] = 'a' + (buf[i] - 'a' + 13) % 26;
                    } else {
                        buf[i] = 'A' + (buf[i] - 'A' + 13) % 26;
                    }
                }
            }
        } else if (strstr(filename, "hex") != NULL) {
            // Dekripsi Hexadecimal
            size_t decoded_size = size / 2;
            char *decoded_text = (char *)malloc(decoded_size);
            memset(decoded_text, 0, decoded_size);

            for (int i = 0, j = 0; i < size; i += 2, j++) {
                char hex[3] = {buf[i], buf[i + 1], '\0'};
                decoded_text[j] = strtol(hex, NULL, 16);
            }

            memcpy(buf, decoded_text, decoded_size);
            free(decoded_text);
        } else if (strstr(filename, "rev") != NULL) {
            // Membalikkan teks
            int i = 0, j = strlen(buf) - 1;
            while (i < j) {
                char temp = buf[i];
                buf[i] = buf[j];
                buf[j] = temp;
                i++;
                j--;
            }
        }
    }
}

// Implementasi eazy read untuk membaca isi file
static int read_eazy(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);
    int fd = open(fpath, O_RDONLY);
    if (fd == -1) return -errno;

    int res = pread(fd, buf, size, offset);
    if (res == -1) {
        close(fd);
        return -errno;
    }
    if(strstr(fpath, "base64") != NULL || strstr(fpath, "rot13") != NULL || strstr(fpath, "hex") != NULL || strstr(fpath, "rev") != NULL)
    {
        decrypt_file_content(path, buf, size);
    }
    close(fd);
    return res;
}

// Implementasi eazy write untuk menulis isi file
static int write_eazy(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);
    int fd = open(fpath, O_WRONLY);
    if (fd == -1) return -errno;

    int res = pwrite(fd, buf, size, offset);
    if (res == -1) res = -errno;
    close(fd);
    return res;
}

static struct fuse_operations operations = {
    .getattr = getattr_eazy,
    .readdir = readdir_eazy,
    .open = open_eazy,
    .mkdir = mkdir_eazy,
    .rmdir = rmdir_eazy,
    .chmod = chmod_eazy,
    .create = create_eazy,
    .unlink = rm_eazy,
    .rename = rename_eazy,
    .read = read_eazy,
    .write = write_eazy,
    };

int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &operations, NULL);
}
