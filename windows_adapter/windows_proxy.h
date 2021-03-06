#ifndef MUSHFS_WINDOWS_PROXY_H
#define MUSHFS_WINDOWS_PROXY_H

#include "../presets.h"

int read_int(int offset);
void write_int(int offset, int data);

void read_struct(int offset, byte* data, int length);
void write_struct(int offset, byte* data, int length);

#endif //MUSHFS_WINDOWS_PROXY_H
