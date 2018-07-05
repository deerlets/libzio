//
// Created by dell on 2018/6/25.
//

#ifndef __LIB_SERIAL_WIN_H
#define __LIB_SERIAL_WIN_H

#if defined(_WIN32)

#include "serial_port.h"

int win32_serial_open(serial_t *ctx);
int win32_serial_set_timeout(serial_t *ctx, int mtimeout);
int win32_serial_read(serial_t *ctx, unsigned char *buf, int len, int mtimeout);
int win32_serial_write(serial_t *ctx, unsigned char *buf, int len, int mtimeout);
int win32_serial_clean_buffer(serial_t *ctx);
int win32_serial_close(serial_t *ctx);
void win32_serial_destory(serial_t *ctx);

#endif

#endif //__LIB_SERIAL_WIN_H
