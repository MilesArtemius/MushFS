#include <stdio.h>
#include "directory.h"

file* find_file_in_dir(string file_name, file* dir) {
    int current_file_page = 0;
    file* current_file = null;
    boolean exists = false;
    directory_entry* entry = malloc(sizeof(directory_entry));
    file_header* header = malloc(sizeof(file_header));
    int files_num = dir->header->size / sizeof(directory_entry);

    for (int i = 0; i < files_num; ++i) {
        read_bytes(dir, (byte*) entry, sizeof(directory_entry), 0);
        get_file_page_header(entry->file_offset, (byte*) header, sizeof(file_header));
        if (equals(header->file_name, file_name)) {
            exists = true;
            current_file_page = entry->file_offset;
            break;
        }
    }
    if (exists) current_file = open_file(current_file_page);

    free(header);
    free(entry);
    return current_file;
}

void add_file_to_dir(file* f, file* dir) {
    directory_entry* entry = malloc(sizeof(directory_entry));
    entry->file_offset = f->main_page;
    append_file(dir, (byte*) entry, sizeof(directory_entry));
    free(entry);
}

void delete_file_from_dir(file* f, file* dir) {
    if (f->header->is_directory) {
        file* current_file = null;
        directory_entry* entry = malloc(sizeof(directory_entry));
        int files_num = f->header->size / sizeof(directory_entry);

        for (int i = 0; i < files_num; ++i) {
            read_bytes(f, (byte*) entry, sizeof(directory_entry), 0);
            current_file = open_file(entry->file_offset);
            delete_file_from_dir(current_file, f);
        }
        free(entry);
        free(current_file);
    }

    directory_entry* entry = malloc(sizeof(directory_entry));
    int files_num = dir->header->size / sizeof(directory_entry);

    seek_to(dir, 0);
    boolean current_file_reached = false;
    int size = sizeof(directory_entry);
    for (int i = 0; i < files_num; ++i) {
        read_bytes(dir, (byte*) entry, size, 0);
        if (!current_file_reached && (entry->file_offset == f->main_page)) {
            current_file_reached = true;
            continue;
        }
        write_bytes(dir, (byte*) entry, size, -size);
    }
    free(entry);

    truncate_file(dir, size);
    delete_file(f);
}

file* open_check_link(file* f) {

}



typedef enum {
    CREATE, OPEN, DELETE
} action;

file* recur_file(mod_string path, file* parent, action act) {
    int delimiter_pos = first_pos(path, delimiter);

    if (delimiter_pos == -1) {
        file* actual = find_file_in_dir(path, parent);
        if (actual == null) {
            if (act == CREATE) {
                actual = create_file(path, parent->main_page);
                add_file_to_dir(actual, parent);
            }
        } else if (act == DELETE) {
            delete_file_from_dir(actual, parent);
            return null;
        } else if (act == OPEN) actual = open_check_link(actual);
        return actual;

    } else {
        mod_string current_dir_path = malloc(delimiter_pos + 1);
        substring_end(path, current_dir_path, delimiter_pos);

        file* current_dir = find_file_in_dir(current_dir_path, parent);
        if (current_dir == null) {
            if (act == CREATE) {
                current_dir = create_file(current_dir_path, parent->main_page);
                current_dir->header->is_directory = true;
                flush_file(current_dir);
                add_file_to_dir(current_dir, parent);
            } else {
                free(current_dir_path);
                free(current_dir);
                return null;
            }
        }

        move_string_by(path, delimiter_pos + 1);
        file* result = recur_file(path, current_dir, act);
        free(current_dir_path);
        free(current_dir);

        return result;
    }
}



file* create_file_global(string path) {
    mod_string copy_path = malloc(len(path) + 1);
    copy_mod(path, copy_path);
    file* result = null;

    if (path[0] == delimiter) {
        file* root = open_file(get_root_dir_number());
        move_string_by(copy_path, 1);
        result = recur_file(copy_path, root, CREATE);
        close_file(root);
    }

    free(copy_path);
    return result;
}

void delete_file_global(string path) {
    mod_string copy_path = malloc(len(path) + 1);
    copy_mod(path, copy_path);

    if (path[0] == delimiter) {
        file* root = open_file(get_root_dir_number());
        move_string_by(copy_path, 1);
        recur_file(copy_path, root, DELETE);
        close_file(root);
    }

    free(copy_path);
}

file* open_file_global(string path) {
    mod_string copy_path = malloc(len(path) + 1);
    copy_mod(path, copy_path);
    file* result = null;

    if (path[0] == delimiter) {
        file* root = open_file(get_root_dir_number());
        move_string_by(copy_path, 1);
        result = recur_file(copy_path, root, OPEN);
        close_file(root);
    }

    free(copy_path);
    return result;
}

file* get_root_dir() {
    return open_file(get_root_dir_number());
}



boolean file_exists(string path) {
    file* f = open_file_global(path);
    boolean answer = f != null;
    close_file(f);
    return answer;
}



int num_files(string path) {
    file* dir = open_file_global(path);
    int files_num = -1;
    if (dir->header->is_directory) files_num = dir->header->size / sizeof(directory_entry);
    close_file(dir);
    return files_num;
}

mod_string* list_files(string dir_path) {
    file* dir = open_file_global(dir_path);
    int files_num = dir->header->size / sizeof(directory_entry);

    mod_string* list = malloc(files_num * sizeof(mod_string));
    directory_entry* entry = malloc(sizeof(directory_entry));
    file_header* header = malloc(sizeof(file_header));

    for (int i = 0; i < files_num; ++i) {
        read_bytes(dir, (byte*) entry, sizeof(directory_entry), 0);
        get_file_page_header(entry->file_offset, (byte*) header, sizeof(file_header));

        int name_length = len(header->file_name);
        list[i] = malloc(name_length + 1);
        substring_end(header->file_name, list[i], name_length);
    }

    free(entry);
    free(header);
    close_file(dir);
    return list;
}

mod_string get_name(file* f) {
    int name_length = len(f->header->file_name);
    mod_string name = malloc(name_length);

    for (int i = 0; i < name_length; ++i) name[i] = f->header->file_name[i];
    name[name_length] = 0;

    return name;
}

mod_string get_path(file* f) {
    if (f->header->parent_page != -1) {
        file* parent = open_file(f->header->parent_page);
        mod_string parent_name = get_path(parent);
        close_file(parent);

        int concat_name_length = len(f->header->file_name) + 1;
        mod_string concat_name = malloc(concat_name_length);
        concatenate("/", f->header->file_name, concat_name);

        int result_name_length = len(parent_name) + concat_name_length;
        mod_string result_name = malloc(result_name_length);
        concatenate(parent_name, concat_name, result_name);
        free(concat_name);
        free(parent_name);

        return result_name;

    } else {
        int name_length = len(f->header->file_name);
        mod_string name = malloc(name_length + 1);
        substring_end(f->header->file_name, name, name_length);
        return name;
    }
}

mod_string get_extension(file* f) {
    mod_string name = get_name(f);
    int extension_length = len(name) - last_pos(name, '.');
    mod_string extension = malloc(extension_length);
    substring_beg(name, extension, len(name) - extension_length + 1);
    return extension;
}
