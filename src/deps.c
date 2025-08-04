#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX

#include "nob.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <curl/curl.h>
#include <archive.h>
#include <archive_entry.h>
#include <zip.h>

#define DEP_DIR "deps"

const char *raylib_url = "https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_linux_amd64.tar.gz";
const char *tiny_file_h = "https://raw.githubusercontent.com/native-toolkit/libtinyfiledialogs/refs/heads/master/tinyfiledialogs.h";
const char *tiny_file_c = "https://raw.githubusercontent.com/native-toolkit/libtinyfiledialogs/refs/heads/master/tinyfiledialogs.c";
const char *font_url = "https://github.com/be5invis/Iosevka/releases/download/v33.2.7/PkgTTF-Iosevka-33.2.7.zip";

const char *raylib_dir = "raylib";
const char *raylib_tar = "raylib.tar.gz";
const char *raylib_ver = "raylib-5.5_linux_amd64";

const char *tiny_dir = "tiny_file_dialog";

const char *font_dir = "font";
const char *font_zip = "PkgTTF-Iosevka-33.2.7.zip";

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    return fwrite(ptr, size, nmemb, stream);
}
int download(const char *url, const char *filename)
{
    CURL *curl = curl_easy_init();
    if (!curl) return 1;

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        return 2;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.79.1");

    char *content_type = NULL;
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, NULL);

    CURLcode res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);
        if (content_type && strstr(content_type, "text/html")) {
            nob_log(NOB_WARNING, "Downloaded file is HTML (not a zip). This may be a redirect or error page.");
        }
    }

    fclose(fp);
    curl_easy_cleanup(curl);
    return res != CURLE_OK;
}

int extract_tar(const char *filename)
{
    struct archive *a = archive_read_new();
    struct archive *ext = archive_write_disk_new();
    archive_read_support_format_tar(a);
    archive_read_support_filter_gzip(a);

    if (archive_read_open_filename(a, filename, 10240)) return 1;

    struct archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        archive_write_header(ext, entry);
        const void *buff;
        size_t size;
        int64_t offset;
        while (archive_read_data_block(a, &buff, &size, &offset) == ARCHIVE_OK)
            archive_write_data_block(ext, buff, size, offset);
        archive_write_finish_entry(ext);
    }

    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);
    return 0;
}

int extract_zip(const char *filename) {
    int err = 0;
    zip_t *za = zip_open(filename, ZIP_RDONLY, &err);
    if (!za) return 1;

    zip_int64_t n = zip_get_num_entries(za, 0);
    for (zip_uint64_t i = 0; i < n; i++) {
        const char *name = zip_get_name(za, i, 0);
        if (!name) continue;

        zip_file_t *zf = zip_fopen_index(za, i, 0);
        if (!zf) continue;

        FILE *out = fopen(name, "wb");
        if (!out) {
            zip_fclose(zf);
            continue;
        }

        char buf[1024];
        zip_int64_t bytes;
        while ((bytes = zip_fread(zf, buf, sizeof(buf))) > 0)
            fwrite(buf, 1, bytes, out);

        fclose(out);
        zip_fclose(zf);
    }

    zip_close(za);
    return 0;
}

void move_fonts(const char *target_dir)
{
    mkdir(target_dir, 0755);
    DIR *d = opendir(".");
    struct dirent *ent;
    while ((ent = readdir(d))) {
        if (strstr(ent->d_name, ".ttf")) {
            char newpath[512];
            snprintf(newpath, sizeof(newpath), "%s/%s", target_dir, ent->d_name);
            rename(ent->d_name, newpath);
        }
    }
    closedir(d);
}

int main(void)
{
    if(!nob_mkdir_if_not_exists(DEP_DIR)) return -1;
    if(!nob_mkdir_if_not_exists(tiny_dir)) return -1;
    printf("----------------------------------------------------\n");
    nob_log(NOB_INFO, "Started Dependency Downloading\n");
    // raylib
    printf("    -------------------------------------------  \n");
    nob_log(NOB_INFO, "RAYLIB Download\n");
    if (download(raylib_url, raylib_tar)) {
        nob_log(NOB_ERROR, "Failed to download raylib");
        return 1;
    }
    nob_log(NOB_INFO, "Extracting Raylib\n");
    if (extract_tar(raylib_tar)) {
        nob_log(NOB_ERROR, "Failed to extract raylib");
        return 1;
    }

    if (rename(raylib_ver, raylib_dir) == 0) {
        nob_log(NOB_INFO, "Raylib Renamed\n");

    } else {
        perror("Error renaming directory");
    }
    nob_log(NOB_INFO, "Raylib successfully setup\n");
    printf("    -------------------------------------------  \n\n");
    //tiny file dialog
    printf("    -------------------------------------------  \n");
    nob_log(NOB_INFO, "TINY FILE DIALOG Download");
    char tiny_c_path[256];
    char tiny_h_path[256];
    snprintf(tiny_c_path, sizeof(tiny_c_path), "%s/tinyfiledialogs.c", tiny_dir);
    snprintf(tiny_h_path, sizeof(tiny_h_path), "%s/tinyfiledialogs.h", tiny_dir);

    if (download(tiny_file_c, tiny_c_path)) {
        nob_log(NOB_ERROR, "Failed to download tinyfiledialogs.c");
        return 1;
    }
    if (download(tiny_file_h, tiny_h_path)) {
        nob_log(NOB_ERROR, "Failed to download tinyfiledialogs.h");
        return 1;
    }
    nob_log(NOB_INFO, "Tiny file dialog successfully setup\n");
    printf("    -------------------------------------------  \n\n");
    // font iosevka
    printf("    -------------------------------------------  \n");
    nob_log(NOB_INFO, "Font downloading\n");
    if (download(font_url, font_zip)) {
        nob_log(NOB_ERROR, "Failed to download font");
        return 1;
    }
    if (extract_zip(font_zip)) {
        nob_log(NOB_ERROR, "Failed to extract font");
        return 1;
    }
    nob_log(NOB_INFO, "Font moving\n");
    move_fonts(font_dir);
    nob_log(NOB_INFO, "Font successfully setup\n");
    printf("    -------------------------------------------  \n\n");
    
    printf("    -------------------------------------------  \n");
    nob_log(NOB_INFO, "Removing the zip and tar files\n");

    rename("raylib", "deps/raylib");
    rename("tiny_file_dialog", "deps/tiny_file_dialog");
    rename("font", "deps/font");
    remove(raylib_tar);
    remove(font_zip);

    nob_log(NOB_INFO, "Set up done\n");
    printf("----------------------------------------------------\n");
    return 0;
}
