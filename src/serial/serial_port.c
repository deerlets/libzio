//
// Created by dell on 2018/6/25.
//
#include "serial_port.h"
#include "serial_public.h"
#include "serial_unix.h"
#include "serial_win.h"
#include <assert.h>
#include <stdio.h>

#if defined(_WIN32)
const serial_backend_t _win_serial_backend = {
	win32_serial_open,
	win32_serial_read,
	win32_serial_write,
	win32_serial_clean_buffer,
	win32_serial_close,
	win32_serial_destory
};
#endif

#if (defined(__unix__) || defined(unix))
const serial_backend_t _linux_serial_backend = {
		unix_serial_open,
		unix_serial_read,
		unix_serial_write,
		unix_serial_clean_buffer,
		unix_serial_close,
		unix_serial_destory
};
#endif

//对外接口定义
serial_t *serial_new(uint8_t index, int baud, char parity,
                     uint8_t data_bit, uint8_t stop_bit, uint8_t serial_mode)
{
	serial_t *ctx;
	serial_rtu_t *ctx_rtu;

	ctx = (serial_t *)malloc(sizeof(serial_t));

#if defined(_WIN32)
	ctx->backend = &_win_serial_backend;
#else
	ctx->backend = &_linux_serial_backend;
#endif

	ctx->backend_data = (serial_rtu_t *)malloc(sizeof(serial_rtu_t));
	ctx_rtu = (serial_rtu_t *)ctx->backend_data;

#if defined(_WIN32)
	sprintf(ctx_rtu->device, "COM%d", index);
#else
	sprintf(ctx_rtu->device, "/dev/ttyS%d", index);
#endif

	ctx_rtu->baud = baud;
	if (parity == 'N' || parity == 'E' || parity == 'O') {
		ctx_rtu->parity = parity;
	} else {
		ctx->backend->destory(ctx);
		return NULL;
	}
	ctx_rtu->data_bit = data_bit;
	ctx_rtu->stop_bit = stop_bit;

	ctx->is_open = false;
	ctx->err_code = 0;
	ctx->debug = 0;

#if defined(_WIN32)
	ctx_rtu->fd = INVALID_HANDLE_VALUE;
#else
	ctx_rtu->s = -1;
#endif

	return ctx;
}

int serial_open(serial_t *ctx)
{
	assert(ctx);
	return ctx->backend->open(ctx);
}

int serial_read(serial_t *ctx, unsigned char *buf, int len, int mtimeout)
{
	assert(ctx);
	return ctx->backend->read(ctx, buf, len, mtimeout);
}

int serial_write(serial_t *ctx, unsigned char *buf, int len, int mtimeout)
{
	assert(ctx);
	return ctx->backend->write(ctx, buf, len, mtimeout);
}

int serial_clean(serial_t *ctx)
{
	assert(ctx);
	return ctx->backend->clean(ctx);
}

int serial_close(serial_t *ctx)
{
	assert(ctx);
	return ctx->backend->close(ctx);
}

void serial_destory(serial_t *ctx)
{
	assert(ctx);
	ctx->backend->destory(ctx);
}

bool serial_is_open(serial_t *ctx)
{
	assert(ctx);
	return ctx->is_open;
}

void serial_set_debug(serial_t *ctx, bool debug)
{
	assert(ctx);
	ctx->debug = debug;
}

int serial_get_last_error(serial_t *ctx)
{
	assert(ctx);
	return ctx->err_code;
}
