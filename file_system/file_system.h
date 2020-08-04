#ifndef MUSHFS_FILE_SYSTEM_H
#define MUSHFS_FILE_SYSTEM_H

#include "../presets.h"

typedef struct {
    int previous;
    int next;
} list_linker;

typedef struct {
    boolean is_empty;
    list_linker linker;
    int head; // WARNING! For empty blocks this int is insignificant!
} block_header;

typedef struct {
    int magic;
    int empty_pages;
    list_linker empty;
    int root_page;
} system_header;



typedef struct {
    byte* page;
    int current, offset;
} data_iterator;



boolean check_drive();
int format_drive();
void check_root();
void print_empty_list();

int get_file_block_head_number(int file_page);
int get_root_dir_number();
boolean is_eof(data_iterator* iterator);
boolean set_offset(data_iterator* iterator, int skip);
boolean add_offset(data_iterator* iterator, int skip);
void reset_and_reload(data_iterator* iterator);

void get_file_header(data_iterator* iterator, byte* container, int length);
void get_file_page_header(int main_page, byte* container, int length);
void set_file_header(data_iterator* iterator, byte* container, int length);

data_iterator* load_data(int main_page);
void dismiss_data(data_iterator* container);
data_iterator* create_data();
void destroy_data(data_iterator* iterator);

void add_page(data_iterator* iterator);
void delete_page(data_iterator* iterator);
void truncate(data_iterator* iterator, int bytes);

boolean get_next_bytes(data_iterator* iterator, byte* container, int length);
boolean get_previous_bytes(data_iterator* iterator, byte* container, int length);

void set_next_bytes(data_iterator* iterator, const byte* container, int length);
void set_previous_bytes(data_iterator* iterator, const byte* container, int length);


#endif //MUSHFS_FILE_SYSTEM_H
