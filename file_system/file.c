#include "file.h"

file* create_file(string name, int parent) {
    file* f = malloc(sizeof(file));
    f->iterator = create_data();
    f->absolute_offset = 0;
    f->main_page = get_file_block_head_number(f->iterator->current);

    f->header = malloc(sizeof(file_header));
    f->header->size = 0;
    f->header->property = DESTROYABLE;
    f->header->is_directory = false;
    f->header->parent_page = parent;
    copy_part(name, f->header->file_name, max_path_len - 1);
    set_file_header(f->iterator, (byte*) f->header, sizeof(file_header));
    set_offset(f->iterator, sizeof(file_header));

    return f;
}

void delete_file(file* f) {
    destroy_data(f->iterator);
    free(f);
}

file* open_file(int page) {
    file* f = malloc(sizeof(file));
    f->iterator = load_data(page);
    f->absolute_offset = 0;
    f->main_page = get_file_block_head_number(f->iterator->current);
    f->header = malloc(sizeof(file_header));
    get_file_header(f->iterator, (byte*) f->header, sizeof(file_header));
    set_offset(f->iterator, sizeof(file_header));
    return f;
}

void close_file(file* f) {
    flush_file(f);
    dismiss_data(f->iterator);
    free(f);
}

void flush_file(file* f) {
    set_file_header(f->iterator, (byte*) f->header, sizeof(file_header));
    reset_and_reload(f->iterator);
    set_offset(f->iterator, sizeof(file_header));
    f->absolute_offset = 0;
}



void append_file(file* f, byte* data, int size) {
    if (f->header->property >= WRITABLE) {
        set_offset(f->iterator, sizeof(file_header) + f->header->size);
        set_next_bytes(f->iterator, data, size);
        f->header->size += size;
        set_file_header(f->iterator, (byte*) f->header, sizeof(file_header));
        f->absolute_offset = f->header->size;
    }
}

void truncate_file(file* f, int bytes) {
    if (f->header->property >= WRITABLE) {
        f->header->size -= bytes;
        set_file_header(f->iterator, (byte *) f->header, sizeof(file_header));
        if (f->absolute_offset > f->header->size) {
            f->absolute_offset = f->header->size;
            set_offset(f->iterator, sizeof(file_header) + f->absolute_offset);
        }
        truncate(f->iterator, bytes);
    }
}

void write_file(file* f, byte* bytes, int size) {
    if (f->header->property >= WRITABLE) {
        set_offset(f->iterator, sizeof(file_header));
        truncate(f->iterator, f->header->size - size);
        f->header->size = size;
        set_file_header(f->iterator, (byte *) f->header, sizeof(file_header));
        set_next_bytes(f->iterator, bytes, size);
        f->absolute_offset = size;
    }
}



void seek_to(file* f, int absolute_position) {
    set_offset(f->iterator, sizeof(file_header) + absolute_position);
    f->absolute_offset = absolute_position;
}

void seek_by(file* f, int absolute_position) {
    add_offset(f->iterator, sizeof(file_header) + absolute_position);
    f->absolute_offset += absolute_position;
}

void write_bytes(file* f, byte* bytes, int size, int offset) {
    if (f->absolute_offset + size > f->header->size) {
        f->header->size = f->absolute_offset + size;
        set_file_header(f->iterator, (byte *) f->header, sizeof(file_header));
    }
    add_offset(f->iterator, offset);
    set_next_bytes(f->iterator, bytes, size);
    f->absolute_offset += offset + size;
}

void read_bytes(file* f, byte* bytes, int size, int offset) {
    add_offset(f->iterator, offset);
    get_next_bytes(f->iterator, bytes, size);
    f->absolute_offset += offset + size;
}
