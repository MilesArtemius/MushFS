#include "windows_proxy.h"
#include "windows.h"

int read_int(int off) {
    int data = 0;
    data |= read_at(off);
    data |= (read_at(off + 1) << 8);
    data |= (read_at(off + 2) << 16);
    data |= (read_at(off + 3) << 24);
    return data;
}

void write_int(int off, int data) {
    write_at(off, data & 0x000000ff);
    write_at(off + 1, (data & 0x0000ff00) >> 8);
    write_at(off + 2, (data & 0x00ff0000) >> 16);
    write_at(off + 3, (data & 0xff000000) >> 24);
}



void read_struct(int offset, byte* data, int length) {
    for (int i = 0; i < length; ++i) {
        data[i] = read_at(offset + i);
    }
}

void write_struct(int offset, byte* data, int length) {
    for (int i = 0; i < length; ++i) {
        write_at(offset + i, data[i]);
    }
}
