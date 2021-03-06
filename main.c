#include <stdio.h>
#include <stdlib.h>
#include "windows_adapter/windows.h"
#include "file_system/file_system.h"
#include "file_system/directory.h"

int main() {
    printf("----- Creating FS -----\n");
    create_fs();

    printf("\n\n\n----- Formatting drive -----\n");
    format_drive();
    printf("\n\n\n----- Checking root -----\n");
    check_root();
    printf("\n\n\n----- Checking empty blocks -----\n");
    print_empty_list();

    printf("\n\n\n----- Reading root -----\n");

    file* stasis = create_file_global("/foo/bar/stasis.lo");
    write_file(stasis, (byte*) "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nunc magna lorem, lobortis sed mattis eu, posuere sit amet eros placerat.", 131);
    stasis = create_file_global("/foo/stasis.li");
    write_file(stasis, (byte*) "stasis captures sliip", 22);
    /*stasis = create_file_global("/foo/lartar/stasis.le");
    write_file(stasis, (byte*) "stasis captures sloop", 22);
    stasis = create_file_global("/grot/stasis.lu");
    write_file(stasis, (byte*) "stasis captures sluup", 22);*/

    printf("\nStasis:\n");
    free(stasis);
    stasis = null;
    stasis = open_file_global("/foo/bar/stasis.lo");
    byte* content = malloc(131);
    read_bytes(stasis, content, 131, 0);
    for (int i = 0; i < 131; ++i) printf("%c", content[i]);
    printf("\n\n");

    printf("Name: %s\n", get_name(stasis));
    printf("Extension: %s\n", get_extension(stasis));
    printf("Path: %s\n", get_path(stasis));

    delete_file_global("/foo/bar");
    printf("\n\n\n----- Checking empty blocks -----\n");
    print_empty_list();

    return 0;
}
