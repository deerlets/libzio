//
// Created by dell on 2018/6/25.
//
#include "serial_port.h"
#include "serial_public.h"
#include "serial_unix.h"
#include "serial_win.h"
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include "../list.h"
//#include "mutex.h"

typedef struct _serial_manger {
	serial_t *t;
	int refcount;
	int opencount;
	pthread_mutex_t mt;
	struct list_head node;
} serial_manger_t;

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

static LIST_HEAD(serial_mangers);
static pthread_mutex_t g_mangers;

static serial_manger_t *find_serial(const char *device)
{
	serial_manger_t *pos;
	list_for_each_entry(pos, &serial_mangers, node) {
		if (strcmp(((serial_rtu_t *)pos->t->backend_data)->device, device) == 0)
			return pos;
	}

	return NULL;
}

static int add_serial(serial_t *serial)
{
	/*serial_manger_t *pos;
	list_for_each_entry(pos, &serial_mangers, node) {
		if (strcmp(((serial_rtu_t *)pos->t->backend_data)->device,
		           ((serial_rtu_t *)serial->backend_data)->device) == 0)
			return -1;
	}*/
	serial_manger_t *sm = malloc(sizeof(serial_manger_t));
	sm->t = serial;
	sm->refcount = 1;
	sm->opencount = 0;
	pthread_mutex_init(&sm->mt, NULL);

	INIT_LIST_HEAD(&sm->node);
	list_add(&sm->node, &serial_mangers);

	return 0;
}

//对外接口定义
serial_t *serial_new(uint8_t index, int baud, char parity,
                     uint8_t data_bit, uint8_t stop_bit, uint8_t share_mode)
{
	char devname[16] = {0};
#if defined(_WIN32)
	sprintf(devname, "COM%d", index);
#else
	sprintf(devname, "/dev/ttyS%d", index);
#endif
	pthread_mutex_lock(&g_mangers);
	serial_manger_t *sm = find_serial(devname);
	if (sm) {
		serial_rtu_t *rtu = (serial_rtu_t *)sm->t->backend_data;
		// 非共享串口
		if (0 == share_mode || 0 == sm->t->share_mode) {
			printf(
				"com%d is busy in use by other but is not share mode(%d, %d)\n",
				index, share_mode, sm->t->share_mode);
			pthread_mutex_unlock(&g_mangers);
			return NULL;
		}
		// 共享串口参数不同
		if (rtu->baud != baud || rtu->parity != parity ||
		    rtu->data_bit != data_bit || rtu->stop_bit != stop_bit) {
			printf("com%d serial port parameter is diverse\n", index);
			pthread_mutex_unlock(&g_mangers);
			return NULL;
		}
		pthread_mutex_lock(&sm->mt);
		++sm->refcount;
		pthread_mutex_unlock(&sm->mt);
		pthread_mutex_unlock(&g_mangers);
		return sm->t;
	}

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
	strcpy(ctx_rtu->device, devname);
	ctx_rtu->baud = baud;
	if (parity == 'N' || parity == 'E' || parity == 'O') {
		ctx_rtu->parity = parity;
	} else {
		ctx->backend->destory(ctx);
		pthread_mutex_unlock(&g_mangers);
		return NULL;
	}
	ctx_rtu->data_bit = data_bit;
	ctx_rtu->stop_bit = stop_bit;

	ctx->is_open = false;
	ctx->err_code = 0;
	ctx->debug = 0;
	ctx->share_mode = share_mode;

#if defined(_WIN32)
	ctx_rtu->fd = INVALID_HANDLE_VALUE;
#else
	ctx_rtu->s = -1;
#endif
	add_serial(ctx);
	pthread_mutex_unlock(&g_mangers);

	return ctx;
}

int serial_open(serial_t *ctx)
{
	assert(ctx);
	serial_manger_t *sm = find_serial(
		((serial_rtu_t *)ctx->backend_data)->device);
	if (!sm)
		return -1;

	// 先判断打开计算
	if (sm->opencount > 0) {
		pthread_mutex_lock(&sm->mt);
		++sm->opencount;
		ctx->is_open = true;
		pthread_mutex_unlock(&sm->mt);
		return 0;
	}
	// 没有打开则打开
	if (0 == ctx->backend->open(ctx)) {
		pthread_mutex_lock(&sm->mt);
		++sm->opencount;
		ctx->is_open = true;
		pthread_mutex_unlock(&sm->mt);
		return 0;
	}

	return -1;
}

int serial_read(serial_t *ctx, unsigned char *buf, int len, int mtimeout)
{
	assert(ctx);
	serial_manger_t *sm = find_serial(
		((serial_rtu_t *)ctx->backend_data)->device);
	if (!sm) {
		return -1;
	}
	//pthread_mutex_lock(&sm->mt);
	int rc = ctx->backend->read(ctx, buf, len, mtimeout);
	//pthread_mutex_unlock(&sm->mt);

	return rc;
}

int serial_write(serial_t *ctx, unsigned char *buf, int len, int mtimeout)
{
	assert(ctx);
	serial_manger_t *sm = find_serial(
		((serial_rtu_t *)ctx->backend_data)->device);
	if (!sm) {
		return -1;
	}
	//pthread_mutex_lock(&sm->mt);
	int rc = ctx->backend->write(ctx, buf, len, mtimeout);
	//pthread_mutex_unlock(&sm->mt);

	return rc;
}

int serial_clean(serial_t *ctx)
{
	assert(ctx);
	serial_manger_t *sm = find_serial(
		((serial_rtu_t *)ctx->backend_data)->device);
	if (!sm) {
		return -1;
	}
	//pthread_mutex_lock(&sm->mt);
	int rc = ctx->backend->clean(ctx);
	//pthread_mutex_unlock(&sm->mt);

	return rc;
}

int serial_close(serial_t *ctx)
{
	assert(ctx);
	serial_manger_t *sm = find_serial(
		((serial_rtu_t *)ctx->backend_data)->device);
	if (!sm) {
		return -1;
	}
	if (sm->opencount > 0){
		pthread_mutex_lock(&sm->mt);
		--sm->opencount;
		pthread_mutex_unlock(&sm->mt);
	}
	if (0 == sm->opencount)
		return ctx->backend->close(ctx);

	return 0;
}

void serial_destory(serial_t *ctx)
{
	assert(ctx);
	pthread_mutex_lock(&g_mangers);
	serial_manger_t *sm = find_serial(
		((serial_rtu_t *)ctx->backend_data)->device);
	if (!sm) {
		free(ctx);
		pthread_mutex_unlock(&g_mangers);
		return;
	}
	if (sm->refcount > 0){
		pthread_mutex_lock(&sm->mt);
		--sm->refcount;
		pthread_mutex_unlock(&sm->mt);
	}

	if (sm->refcount == 0) {
		list_del(&sm->node);
		ctx->backend->destory(ctx);
		free(sm);
	}
	pthread_mutex_unlock(&g_mangers);
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

void serial_lock(serial_t *ctx)
{
    assert(ctx);
    serial_manger_t *sm = find_serial(
            ((serial_rtu_t *)ctx->backend_data)->device);
    if (!sm) {
        return;
    }
    pthread_mutex_lock(&sm->mt);
}

void serial_unlock(serial_t *ctx)
{
    assert(ctx);
    serial_manger_t *sm = find_serial(
            ((serial_rtu_t *)ctx->backend_data)->device);
    if (!sm) {
        return;
    }
    pthread_mutex_unlock(&sm->mt);
}