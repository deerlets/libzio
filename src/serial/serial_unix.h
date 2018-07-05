//
// Created by dell on 2018/6/25.
//

#ifndef __LIB_SERIAL_UNIX_H
#define __LIB_SERIAL_UNIX_H

#include "serial_port.h"

#if (defined(__unix__) || defined(unix))

#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int unix_serial_open(serial_t *ctx);
int unix_serial_set_timeout(serial_t *ctx, int mtimeout);
int unix_serial_read(serial_t *ctx, unsigned char *buf, int len, int mtimeout);
int unix_serial_write(serial_t *ctx, unsigned char *buf, int len, int mtimeout);
int unix_serial_clean_buffer(serial_t *ctx);
int unix_serial_close(serial_t *ctx);
void unix_serial_destory(serial_t *ctx);

#endif //_UNIX
#endif //__LIB_SERIAL_UNIX_H
