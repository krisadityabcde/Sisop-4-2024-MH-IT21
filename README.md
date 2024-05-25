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
Pada folder "pesan" Adfi ingin meningkatkan kemampuan sistemnya dalam mengelola berkas-berkas teks dengan menggunakan fuse.
- Jika sebuah file memiliki prefix "base64," maka sistem akan langsung mendekode isi file tersebut dengan algoritma Base64.
- Jika sebuah file memiliki prefix "rot13," maka isi file tersebut akan langsung di-decode dengan algoritma ROT13.
- Jika sebuah file memiliki prefix "hex," maka isi file tersebut akan langsung di-decode dari representasi heksadesimalnya.
- Jika sebuah file memiliki prefix "rev," maka isi file tersebut akan langsung di-decode dengan cara membalikkan teksnya.

Untuk soal ini, saya membuat sebuah function untuk mendekripsi pesan berdasarkan nama file yang diambil dari pathnya
```c
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
```

Di sini, setiap file akan didekripsi menurut algoritmanya masing-masing. Lalu, untuk pemanggilan fungsinya saya lakukan di function read agar dapat dieksekusi ketika melakukan `cat` untuk membaca isi file txt.
```c
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
```

Pada folder “rahasia-berkas”, Adfi dan timnya memutuskan untuk menerapkan kebijakan khusus. Mereka ingin memastikan bahwa folder dengan prefix "rahasia" tidak dapat diakses tanpa izin khusus. 
Jika seseorang ingin mengakses folder dan file pada “rahasia”, mereka harus memasukkan sebuah password terlebih dahulu (password bebas).
Selanjutnya, untuk bagian ini saya membuat sebuah function yang akan melakukan printf perintah permintaan password dan scanf untuk mengambil input pengguna, yang apabila password yang dimasukkan salah maka akses akan ditolak.
```c
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
```

Untuk function ini sendiri saya buat pemanggilannya di dalam function `readdir` dan `open` yang akan menghasilkan pemanggilan password ketika pengguna ingin melakukan `ls` di dalam folder "rahasia" dan membuka file apapun di dalamnya.
Sebagai tambahan, agar bisa melakukan printf dan scanf maka program fuse ini diharuskan berjalan di foreground dengan menggunakan command `./pastibisa -f /path/to/fuse/folder` sehingga program dapat melakukan stdin dan stdout. Sementara itu untuk mengakses folder fuse bisa dilakukan dari terminal lain yang dibuka bersamaan.

### 2c
Setiap proses yang dilakukan akan tercatat pada logs-fuse.log dengan format :
[SUCCESS/FAILED]::dd/mm/yyyy-hh:mm:ss::[tag]::[information]
Ex:
[SUCCESS]::01/11/2023-10:43:43::[moveFile]::[File moved successfully]

```c
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
```
Fungsi pembuatan log sama seperti biasanya.

### Screenshot hasil pengerjaan

### Kendala
Tidak ada kendala

### Revisi
Tidak ada revisi

## Soal 3
### 3a
Buatlah sebuah direktori dengan ketentuan seperti pada tree berikut
.
├── [nama_bebas]
├── relics
│   ├── relic_1.png.000
│   ├── relic_1.png.001
│   ├── dst dst…
│   └── relic_9.png.010
└── report

### 3b
Direktori [nama_bebas] adalah direktori FUSE dengan direktori asalnya adalah direktori relics. Ketentuan Direktori [nama_bebas] adalah sebagai berikut :
- Ketika dilakukan listing, isi dari direktori [nama_bebas] adalah semua relic dari relics yang telah tergabung.
Untuk bagian ini, saya melakukan modifikasi pada function `readdir` dengan mengambil file yang memiliki nama yang saya sembari mengurutkan nomor pecahannya di belakang. Ketika sudah diambil lalu file akan ditulis ulang menjadi satu kesatuan yang utuh di dalam sebuah temporary directory `/tmp/tempe/` kemudian path dari file yang digabungkan akan disimpan dalam sebuah linkedlist agar nantinya setelah program fuse berhasil dijalankan, file hasil penggabungan dapat dihapus dan tidak tertinggal di dalam temporary directory.
```c
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
```
```c
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
```

- Ketika dilakukan copy (dari direktori [nama_bebas] ke tujuan manapun), file yang disalin adalah file dari direktori relics yang sudah tergabung.
Pada bagian ini, modifikasi dilakukan pada fungsi `create` yang konsepnya adalah membaca isi dari temporary file yang mana berisi file yang sudah digabungkan sebelumnya, dan file tersebutlah yang akan dicopy ke destinasi tujuan sesuai dengan keinginan pengguna.
```c
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
```

- Ketika ada file dibuat, maka pada direktori asal (direktori relics) file tersebut akan dipecah menjadi sejumlah pecahan dengan ukuran maksimum tiap pecahan adalah 10kb. File yang dipecah akan memiliki nama [namafile].000 dan seterusnya sesuai dengan jumlah pecahannya.
Untuk bagian soal ini, modifikasi dilakukan pada function `write` di mana setelah file dibaca dari temporary directory, file tidak akan langsung ditulis begitu saja namun akan dipecah menjadi pecahan berukuran 10kB dan ditulis satu persatu hingga keseluruhan file berhasil dipecah.
```c
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
```

- Ketika dilakukan penghapusan, maka semua pecahannya juga ikut terhapus. Untuk bagian ini akan sedikit berbeda, jika sebelumnya yang selalu dimention adalah temporary directory, maka function unlink di sini akan menghapus dari source directory tempat semua pecahan file berada, dan akan menghapus semua pecahan dari file yang ignin dihapus
```c
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
```

### 3c
Direktori report adalah direktori yang akan dibagikan menggunakan Samba File Server. Setelah kalian berhasil membuat direktori [nama_bebas], jalankan FUSE dan salin semua isi direktori [nama_bebas] pada direktori report.
Cara melakukannya hanya dengan command copy ke dalam direktori report kemudian menginisiasi Samba sesuai denga nmodul.

### Screenshot hasil pengerjaan

### Kendala
Tidak ada kendala

### Revisi
Tidak ada revisi
