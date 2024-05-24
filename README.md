# Sisop-4-2024-MH-IT21
## Anggota Kelompok:
- Rafael Ega Krisaditya	(5027231025)
- Rama Owarianto Putra Suharjito	(5027231049)
- Danar Bagus Rasendriya	(5027231055)

## Soal 1
### 1a
Membuat folder dengan prefix "wm" Dalam folder ini, setiap gambar yang dipindahkan ke dalamnya akan diberikan watermark bertuliskan "inikaryakita.id"

Untuk mengerjakan bagian soal ini, saya memodifikasi function fuse untuk `rename`
```c
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
```
Pada function ini, ketika command rename atau mv diinisiasi, maka program akan melihat terlebih dahulu pada path tujuan, apabila kemudian di dalam path tersebut mengandung string "wm" yang mana adalah folder khusus untuk gambar yang diberikan watermark, maka file akan dibuka sebelum dipindahkan

```c
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
```
Lalu, untuk membuat watermark saya menggunakan sebuah command third party yakni ImageMagick yang bsia diinstal di terminal menggunakan `sudo apt install imagemagick` yang dapat diinisiasi menggunakan command `convert`. Saya membuat command line dalam bentuk string yang nantinya akan dijalankan menggunakan function `system()` lengkap dengan ketentuan yang saya inginkan termasuk ukuran, letak, warna, dan teks watermarknya

```c
 char watermark_text[] = "inikaryakita.id";
        char command[1000];
        sprintf(command, "convert -gravity south -geometry +0+20 /proc/%d/fd/%d -fill white -pointsize 36 -annotate +0+0 '%s' /proc/%d/fd/%d", getpid(), srcfd, watermark_text, getpid(), destfd);

        int res = system(command);
        if (res == -1) {
            perror("Error: Failed to execute ImageMagick command.");
            close(srcfd);
            close(destfd);
            return -errno;
```
Setelah diberikan watermark barulah gambar dipindahkan dengan `srcfd` dan `destfd` yang sebelumnya sudah dibuka. Sedangkan untuk file yang tidak dipindahkan ke folder wm maka akan langsung dipindahkan dengan syntax `rename()`

### 1b
Mereka harus mengubah permission pada file "script.sh" agar bisa dijalankan, karena jika dijalankan maka dapat menghapus semua dan isi dari  "gallery". Pada soal ini saya sempat salah paham bahwa file script.sh seharusnya diberikan izin karena saya berpikir bahwa sesuatu yang bahaya seharusnya bisa langsung ditolak aksesnya agar tidak bisa dijalankan oleh siapapun, maka dari itu kode saya akan membaca kembail path dari file yang dimaksud dan jika mengandung string "script.sh" maka aksesnya akan langsung ditolak
```c
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
```

Tapi karena file harus dijalankan, maka tinggal menghapus bagian `if(strstr...)` saja

### 1c
Adfi dan timnya juga ingin menambahkan fitur baru dengan membuat file dengan prefix "test" yang ketika disimpan akan mengalami pembalikan (reverse) isi dari file tersebut.  
Untuk mengerjakan soal ini, ada 3 function yang saya modifikasi yakni `read` `write` dan `create`. Pemakaiannya, ketika file sudah disediakan dan ada di dalam direktori maka yang diakses adalah `read` sementara untuk `write` berguna agar di direktori asal fuse file yang dibuat menggunakan `create` tetap terbaik sehingga hanya bisa dibaca di dalam direktori fuse. Konsepnya untuk ketiga function ini sebenarnya sama yaitu menggunakan `if(strstr...)` untuk melihat pathfile ketika mengandung string "test" maka function untuk membalik isi file akan dipanggil
Untuk function `read` dan `write` saya membaut function tambahan ini
```c
static int reverse_buffer(char *buf, size_t size) {
    for (size_t i = 0; i < size / 2; i++) {
        char temp = buf[i];
        buf[i] = buf[size - i - 1];
        buf[size - i - 1] = temp;
    }
    return 0;
}
```

Sementara untuk `create` saya menggunakan function ini
```c
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
```

### Screenshot hasil pengerjaan

### Kendala
Tidak ada kendala

### Revisi
Tidak ada revisi

## Soal 2
### 2a

