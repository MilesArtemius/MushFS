#include "file_system.h"
#include "../windows_adapter/windows.h"
#include "../windows_adapter/windows_proxy.h"
#include "../utils/math_utils.h"
#include "directory.h"
#include <stdlib.h>
#include <stdio.h>

const int magic = 0x10afc01;
const int offset = 0; //0x1000;
const int page_size = 128; //1024;
const int page_content_size = 128 - sizeof(block_header);



int get_page_offset(int page_num) {
    return offset + sizeof(system_header) + (page_num * page_size);
}

int get_page_content_offset(int page_num) {
    return offset + sizeof(system_header) + (page_num * page_size) + sizeof(block_header);
}

boolean check_drive() {
    return magic == read_int(offset);
}



int get_next_block_number(int current) {
    block_header* header = malloc(sizeof(block_header));
    read_struct(get_page_offset(current), (byte*) header, sizeof(block_header));
    int next = header->linker.next;
    free(header);
    return next;
}

int get_previous_block_number(int current) {
    block_header* header = malloc(sizeof(block_header));
    read_struct(get_page_offset(current), (byte*) header, sizeof(block_header));
    int next = header->linker.previous;
    free(header);
    return next;
}

int get_file_block_head_number(int file_page) {
    block_header* current = malloc(sizeof(block_header));
    read_struct(get_page_offset(file_page), (byte*) current, sizeof(block_header));
    int main = current->head;
    free(current);
    return main;
}

// Call to occupy a block, it returns the block-to-occupy number;
int occupy_block() {
    system_header* header = malloc(sizeof(system_header));
    read_struct(offset, (byte*) header, sizeof(system_header));

    int free_block = header->empty.previous;
    header->empty.previous = get_next_block_number(header->empty.previous);
    header->empty_pages--;
    write_struct(offset, (byte *) header, sizeof(system_header));

    block_header *block = malloc(sizeof(block_header));
    read_struct(get_page_offset(header->empty.previous), (byte *) block, sizeof(block_header));
    block->linker.previous = -1;
    write_struct(get_page_offset(header->empty.previous), (byte *) block, sizeof(block_header));

    free(block);
    free(header);
    return free_block;
}

// Call to free a block with free_block = freed block number;
void free_block(int free_block) {
    system_header* header = malloc(sizeof(system_header));
    read_struct(offset, (byte*) header, sizeof(system_header));

    int previous_last = header->empty.next;
    header->empty.next = free_block;
    header->empty_pages++;
    write_struct(offset, (byte *) header, sizeof(system_header));

    block_header *block = malloc(sizeof(block_header));
    read_struct(get_page_offset(previous_last), (byte *) block, sizeof(block_header));
    block->linker.next = free_block;
    write_struct(get_page_offset(previous_last), (byte *) block, sizeof(block_header));

    read_struct(get_page_offset(header->empty.next), (byte *) block, sizeof(block_header));
    block->is_empty = true;
    block->linker.previous = previous_last;
    block->linker.next = -1;
    write_struct(get_page_offset(header->empty.next), (byte *) block, sizeof(block_header));

    free(block);
    free(header);
}



int format_drive() {
    long available_size = get_drive_size() - offset;
    if (available_size <= 0) return 1;

    system_header* header = malloc(sizeof(system_header));
    header->magic = magic;
    header->root_page = offset + sizeof(system_header);
    header->empty_pages = available_size / page_size - 1;
    header->empty.previous = 0;
    header->empty.next = header->empty_pages - 1;
    write_struct(offset, (byte*) header, sizeof(system_header));

    block_header* empty_block = malloc(sizeof(block_header));
    empty_block->is_empty = true;
    for (int i = header->empty.previous; i < header->empty.next; ++i) {
        empty_block->linker.previous = i - 1;
        empty_block->linker.next = i + 1;
        write_struct(get_page_offset(i), (byte*) empty_block, sizeof(block_header));
    }
    empty_block->linker.previous = header->empty.next - 1;
    empty_block->linker.next = -1;
    write_struct(get_page_offset(header->empty.next), (byte*) empty_block, sizeof(block_header));
    free(empty_block);
    write_struct(offset, (byte*) header, sizeof(system_header));

    int root_block_num = occupy_block();
    read_struct(offset, (byte*) header, sizeof(system_header));
    header->root_page = root_block_num;
    write_struct(offset, (byte*) header, sizeof(system_header));
    int current_offset = get_page_offset(header->root_page);

    block_header* root_block = malloc(sizeof(block_header));
    root_block->is_empty = false;
    root_block->linker.previous = root_block->linker.next = -1;
    root_block->head = header->root_page;
    write_struct(current_offset, (byte*) root_block, sizeof(block_header));
    current_offset += sizeof(block_header);

    file_header* root_header = malloc(sizeof(file_header));
    root_header->size = 0;
    root_header->property = WRITABLE;
    root_header->is_directory = true;
    root_header->parent_page = -1;
    copy_part("root", root_header->file_name, max_path_len - 1);
    write_struct(current_offset, (byte*) root_header, sizeof(file_header));

    free(root_block);
    free(root_header);
    free(header);
    return 0;
}



boolean data_step(data_iterator* iterator, boolean flush_page) {
    while (iterator->offset >= page_content_size) {
        int next = get_next_block_number(iterator->current);
        if (flush_page) write_struct(get_page_content_offset(iterator->current), iterator->page, page_content_size);
        if (next == -1) return true;
        iterator->offset = 0;
        iterator->current = next;
        read_struct(get_page_content_offset(iterator->current), iterator->page, page_content_size);
    }
    while (iterator->offset < 0) {
        int previous = get_previous_block_number(iterator->current);
        if (flush_page) write_struct(get_page_content_offset(iterator->current), iterator->page, page_content_size);
        if (previous == -1) return true;
        iterator->offset = page_content_size - 1;
        iterator->current = previous;
        read_struct(get_page_content_offset(iterator->current), iterator->page, page_content_size);
    }
    return false;
}



int get_root_dir_number() {
    system_header* header = malloc(sizeof(system_header));
    read_struct(offset, (byte*) header, sizeof(system_header));
    int root = header->root_page;
    free(header);
    return root;
}

boolean is_eof(data_iterator* iterator) {
    int next = 0;
    if (iterator->offset >= page_content_size) next = get_next_block_number(iterator->current);
    else if (iterator->offset < 0) next = get_previous_block_number(iterator->current);
    return next == -1;
}

boolean set_offset(data_iterator* iterator, int skip) {
    iterator->current = get_file_block_head_number(iterator->current);
    read_struct(get_page_content_offset(iterator->current), iterator->page, page_content_size);
    iterator->offset = skip;
    return data_step(iterator, false);
}

boolean add_offset(data_iterator* iterator, int skip) {
    iterator->offset += skip;
    return data_step(iterator, false);
}

void reset_and_reload(data_iterator* iterator) {
    *iterator = *load_data(get_file_block_head_number(iterator->current));
}



void get_file_header(data_iterator* iterator, byte* container, int length) {
    read_struct(get_page_content_offset(get_file_block_head_number(iterator->current)), container, length);
}

void get_file_page_header(int main_page, byte* container, int length) {
    read_struct(get_page_content_offset(main_page), container, length);
}

void set_file_header(data_iterator* iterator, byte* container, int length) {
    if (iterator->current == get_file_block_head_number(iterator->current)) for (int i = 0; i < length; ++i) iterator->page[i] = container[i];
    write_struct(get_page_content_offset(get_file_block_head_number(iterator->current)), container, length);
}



data_iterator* load_data(int main_page) {
    data_iterator* iterator = malloc(sizeof(data_iterator));
    iterator->offset = 0;
    iterator->current = main_page;
    iterator->page = malloc(page_content_size);
    read_struct(get_page_content_offset(iterator->current), iterator->page, page_content_size);
    return iterator;
}

void dismiss_data(data_iterator* container) {
    free(container->page);
    free(container);
}

data_iterator* create_data() {
    int new_block = occupy_block();
    block_header *block = malloc(sizeof(block_header));
    read_struct(get_page_offset(new_block), (byte *) block, sizeof(block_header));
    block->is_empty = false;
    block->head = new_block;
    block->linker.previous = block->linker.next = -1;
    write_struct(get_page_offset(new_block), (byte *) block, sizeof(block_header));
    return load_data(new_block);
}

void destroy_data(data_iterator* iterator) {
    block_header *block = malloc(sizeof(block_header));
    read_struct(get_page_offset(get_file_block_head_number(iterator->current)), (byte *) block, sizeof(block_header));
    int next_page = block->linker.next;
    free_block(get_file_block_head_number(iterator->current));
    while (next_page != -1) {
        read_struct(get_page_offset(next_page), (byte *) block, sizeof(block_header));
        int current_page = next_page;
        next_page = block->linker.next;
        free_block(current_page);
    }
    free(block);
    dismiss_data(iterator);
}



void add_page(data_iterator* iterator) {
    block_header *block = malloc(sizeof(block_header));
    read_struct(get_page_offset(iterator->current), (byte*) block, sizeof(block_header));
    int current_page = iterator->current, next_page = block->linker.next, main_page = block->head;
    while (next_page != -1) {
        read_struct(get_page_offset(next_page), (byte *) block, sizeof(block_header));
        current_page = next_page;
        next_page = block->linker.next;
    }

    int new_block = occupy_block();
    read_struct(get_page_offset(current_page), (byte*) block, sizeof(block_header));
    block->linker.next = new_block;
    write_struct(get_page_offset(current_page), (byte*) block, sizeof(block_header));

    read_struct(get_page_offset(new_block), (byte*) block, sizeof(block_header));
    block->is_empty = false;
    block->head = main_page;
    block->linker.previous = current_page;
    block->linker.next = -1;
    write_struct(get_page_offset(new_block), (byte*) block, sizeof(block_header));

    free(block);
}

void delete_page(data_iterator* iterator) {
    block_header *block = malloc(sizeof(block_header));
    read_struct(get_page_offset(iterator->current), (byte *) block, sizeof(block_header));
    int next_page = block->linker.next;
    while (next_page != -1) {
        read_struct(get_page_offset(next_page), (byte *) block, sizeof(block_header));
        next_page = block->linker.next;
    }

    int previous_page = block->linker.previous;
    read_struct(get_page_offset(previous_page), (byte *) block, sizeof(block_header));
    free_block(block->linker.next);
    block->linker.next = -1;
    write_struct(get_page_offset(previous_page), (byte *) block, sizeof(block_header));

    free(block);
}

void truncate(data_iterator* iterator, int bytes) {
    int free_pages = bytes / page_content_size;
    for (int i = 0; i < free_pages; ++i) delete_page(iterator);
}



boolean get_next_bytes(data_iterator* iterator, byte* container, int length) {
    for (int i = 0; i < length; ++i) {
        if (data_step(iterator, false)) return true;
        container[i] = iterator->page[iterator->offset];
        iterator->offset++;
    }
    return false;
}

boolean get_previous_bytes(data_iterator* iterator, byte* container, int length) {
    for (int i = length; i >= 0; --i) {
        if (data_step(iterator, false)) return true;
        container[i] = iterator->page[iterator->offset];
        iterator->offset--;
    }
    return false;
}



void set_next_bytes(data_iterator* iterator, const byte* container, int length) {
    for (int i = 0; i < length; ++i) {
        while (data_step(iterator, true)) add_page(iterator);
        iterator->page[iterator->offset] = container[i];
        iterator->offset++;
    }
    write_struct(get_page_content_offset(iterator->current), iterator->page, page_content_size);
}

void set_previous_bytes(data_iterator* iterator, const byte* container, int length) {
    for (int i = length; i >= 0; --i) {
        while (data_step(iterator, true)) add_page(iterator);
        iterator->page[iterator->offset] = container[i];
        iterator->offset--;
    }
    write_struct(get_page_content_offset(iterator->current), iterator->page, page_content_size);
}



void check_root() {
    boolean drive_valid = check_drive();
    printf("Drive valid: %d\n", check_drive());
    if (!drive_valid) return;

    system_header* header = malloc(sizeof(system_header));
    read_struct(offset, (byte*) header, sizeof(system_header));
    printf("\nSystem header info:\n");
    printf("\tEmpty pages: %d\n", header->empty_pages);
    printf("\tRoot offset: %d\n", header->root_page);
    printf("\tFirst empty: %d\n", header->empty.previous);
    printf("\tLast empty: %d\n", header->empty.next);

    block_header* root_dir_block = malloc(sizeof(block_header));
    read_struct(get_page_offset(header->root_page), (byte*) root_dir_block, sizeof(block_header));
    printf("\nRoot dir block info:\n");
    printf("\tIs empty: %d\n", root_dir_block->is_empty);
    printf("\tNext block: %d; previous block: %d\n", root_dir_block->linker.next, root_dir_block->linker.previous);
    printf("\tHead block: %d\n", root_dir_block->head);

    file_header* root_dir_file = malloc(sizeof(file_header));
    read_struct(get_page_content_offset(header->root_page), (byte*) root_dir_file, sizeof(file_header));
    printf("\nRoot dir file_header info:\n");
    printf("\tSize: %d\n", root_dir_file->size);
    printf("\tProperty: %d\n", root_dir_file->property);
    printf("\tParent page: %d\n", root_dir_file->parent_page);
    printf("\tPath: %s (", root_dir_file->file_name);
    for (int i = 0; i < 16; ++i) printf("%d ", root_dir_file->file_name[i]);
    printf(")\n");

    file_header* root_dir = malloc(sizeof(file_header));
    read_struct(get_page_content_offset(header->root_page) + sizeof(file_header), (byte*) root_dir, sizeof(file_header));
    printf("\nRoot dir info:\n");

    free(header);
    free(root_dir_block);
    free(root_dir_file);
    free(root_dir);
}

void print_empty_list() {
    system_header *header = malloc(sizeof(system_header));
    block_header *block = malloc(sizeof(block_header));
    read_struct(offset, (byte *) header, sizeof(system_header));

    printf("Total: %d\n", header->empty_pages);
    printf("First: %d, Last: %d\n", header->empty.previous, header->empty.next);
    printf("Here:\n");
    int next_block_num = header->empty.previous;
    do {
        printf("%d -> ", next_block_num);
        read_struct(get_page_offset(next_block_num), (byte *) block, sizeof(block_header));
        next_block_num = block->linker.next;
    } while (next_block_num != header->empty.next);
    printf("%d\n", next_block_num);

    printf("There:\n");
    next_block_num = header->empty.next;
    do {
        printf("%d <- ", next_block_num);
        read_struct(get_page_offset(next_block_num), (byte *) block, sizeof(block_header));
        next_block_num = block->linker.previous;
    } while (next_block_num != header->empty.previous);
    printf("%d\n", next_block_num);

    free(header);
    free(block);
}
