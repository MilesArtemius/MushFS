#ifndef MUSHFS_FILE_H
#define MUSHFS_FILE_H

#include "../presets.h"
#include "file_system.h"
#include "../utils/string_utils.h"
#include <stdlib.h>

#define max_path_len 16

typedef enum {
    READABLE, WRITABLE, DESTROYABLE, EXECUTABLE
} user_mode;

typedef struct {
    int size;
    user_mode property;
    boolean is_directory;
    int parent_page;
    char file_name [max_path_len]; //[256];
} file_header;

typedef struct {
    file_header* header;
    int main_page;
    int absolute_offset;
    data_iterator* iterator;
} file;



file* create_file(string name, int parent);
void delete_file(file* f);

file* open_file(int page);
void close_file(file* f);

void flush_file(file* f);

void append_file(file* f, byte* data, int size);
void truncate_file(file* f, int bytes);
void write_file(file* f, byte* bytes, int size);

void seek_to(file* f, int absolute_position);
void seek_by(file* f, int absolute_position);
void write_bytes(file* f, byte* bytes, int size, int offset);
void read_bytes(file* f, byte* bytes, int size, int offset);

#endif //MUSHFS_FILE_H
