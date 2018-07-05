//
// Created by dell on 2018/6/26.
//

#ifndef __LIB_SERIAL_PORT_H
#define __LIB_SERIAL_PORT_H

//#define _UNIX

#include <stdint.h>
#include "serial_public.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <termios.h>
#endif

#define RS232 0
#define RS485 1

typedef struct _serial_rtu {
	char device[16];
	int baud;
	uint8_t data_bit;
	uint8_t stop_bit;
	char parity;
#if defined(_WIN32)
	HANDLE fd;
	DCB old_dcb;
#endif //_WIN32

#if (defined(__unix__) || defined(unix))
    struct termios old_tios;
	int s;
#endif //_UNIX

#if HAVE_DECL_TIOCSRS485
	int serial_mode;
#endif

} serial_rtu_t;

typedef struct _serial_backend {
	int (*open) (serial_t *ctx);
	int (*read) (serial_t *ctx, unsigned char *buf, int len, int timeout);
	int (*write) (serial_t *ctx, unsigned char *buf, int len, int timeout);
	int (*clean)(serial_t *ctx);
	int (*close) (serial_t *ctx);
	void (*destory) (serial_t *ctx);

} serial_backend_t;

struct _serial {
	int debug;
	bool is_open;
	int err_code;
	const serial_backend_t *backend;
	void *backend_data;
};

#endif //__LIB_SERIAL_PORT_H
