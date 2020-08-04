#ifndef MUSHFS_DIRECTORY_H
#define MUSHFS_DIRECTORY_H

#include "file.h"

#define delimiter '/'

typedef struct {
    int file_offset;
} directory_entry;



file* create_file_global(string path);
void delete_file_global(string path);
file* open_file_global(string path);
file* get_root_dir();

boolean file_exists(string path);

int num_files(string path);
mod_string* list_files(string dir_path);

mod_string get_name(file* f);
mod_string get_path(file* f);
mod_string get_extension(file* f);

#endif //MUSHFS_DIRECTORY_H
