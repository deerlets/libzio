//
// Created by dell on 2018/7/2.
//

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "socket_cli.h"
#include "socket_public.h"

static void *task_read(void *arg)
{
	socket_cli_t *cli = (socket_cli_t *)arg;
	socket_data_t *data = (socket_data_t *)cli->backend_data;

#ifdef unix
	prctl(PR_SET_NAME, "socket_cli", NULL, NULL, NULL);
#endif

	while (cli->is_connect) {
		char cbuf[4096] = {0};
		int ret = recv(data->socket_id, cbuf, 4096, 0);
		if (ret > 0) {
			data->recv_cb(cbuf, ret);
			if (cli->debug)
				printf("recv: %s \n", cbuf);
		}
	}

	return NULL;
}

static int __init()
{
#if defined(_WIN32)
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA data;
	if (WSAStartup(sockVersion, &data) != 0) {
		printf("WSAStartup error<MAKEWORD(2,2)>, error code is %d\n",
		       WSAGetLastError());
		return -1;
	}
#endif

	return 0;
}

static int __get_cur_err()
{
#if defined(_WIN32)
	return WSAGetLastError();
#else
	return errno;
#endif

	return 0;
}

static int __select_out(socket_cli_t *s, int timeout)
{
	struct timeval timout;
	fd_set wset, rset, eset;
	int ret, maxfdp1;
	socket_data_t *data = (socket_data_t *)s->backend_data;

	if (timeout == 0) return 1;
	/* setup sockets to read */
	maxfdp1 = data->socket_id + 1;
	FD_ZERO(&rset);
	FD_SET (data->socket_id, &rset);
	FD_ZERO(&wset);
	FD_ZERO(&eset);
	timout.tv_sec = timeout / 1000;
	timout.tv_usec = (timeout % 1000) * 1000;

	ret = select(maxfdp1, &rset, &wset, &eset, &timout);
	if (ret == 0) {
		s->err_code = __get_cur_err();
		//printf("select timeout\n");
		return -1; /* timeout */
	}

	return 0;
}

static int __connect_tcp(socket_cli_t *s)
{
	if (s->is_connect) { return -1; }

	socket_data_t *data = (socket_data_t *)s->backend_data;

	data->socket_id = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == data->socket_id) {
		printf("invalid socket ! \n");
		s->err_code = __get_cur_err();
		return -1;
	}

	if (-1 == connect(data->socket_id, (struct sockaddr *)&data->remote_addr,
	                  sizeof(data->remote_addr))) {
		s->err_code = __get_cur_err();
		printf("connect server error, error_code %d\n", s->err_code);
		return -1;
	}

	s->is_connect = true;

	return 0;
}

static int __read_tcp(socket_cli_t *s, void *buf, int len, int timeout)
{
	if (!s->is_connect) {
		printf("is not connect server \n");
		return -1;
	}
	if (s->async) {
		printf("async mode by socket_register_notify to read. \n");
		return -1;
	}

	/*if (-1 == __select_out(s, timeout)) {
		printf(
			"timeout, not bytes to read, Maybe disconnect from server or server is close.\n");
		return -1; // timeout
	}*/

	socket_data_t *data = (socket_data_t *)s->backend_data;

	char *cbuf = (char *)buf;
	int read_len = 0;

	read_len = recv(data->socket_id, &cbuf[read_len], len - read_len, 0);
	if (read_len == -1) {
		s->err_code = __get_cur_err();
		printf("read buf [%s], error code [%d]\n", cbuf, s->err_code);
		return -1;
	}

	if (s->debug)
		printf("read over [%s] \n", cbuf);

	return read_len;
}

static int __write_tcp(socket_cli_t *s, void *buf, int len, int timeout)
{
	if (!s->is_connect) return -1;

	socket_data_t *data = (socket_data_t *)s->backend_data;

	int ret, bytes_left, write_bytes;
	const char *cbuf;
	cbuf = (char *)buf;
	bytes_left = len;
	write_bytes = 0;

	while (bytes_left > 0) {
		ret = send(data->socket_id, &cbuf[write_bytes], bytes_left, 0);
		if (ret <= 0) {
			s->err_code = __get_cur_err();
			printf("write error, write lenth = %d, write error code =%d\n",
			       write_bytes, s->err_code);
			return -1;
		}
		bytes_left -= ret;
		write_bytes += ret;
	}

	if (s->debug)
		printf("write over [%s] \n", cbuf);
	return write_bytes;
}

static int __clean_tcp(socket_cli_t *s)
{
	if (!s->is_connect) return -1;
	if (!s->async) return 0;

	socket_data_t *data = (socket_data_t *)s->backend_data;

	struct timeval tmOut;
	tmOut.tv_sec = 0;
	tmOut.tv_usec = 0;
	int nRet, maxfdp1;
	maxfdp1 = data->socket_id + 1;
	fd_set wset, rset, eset;
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_ZERO(&eset);
	FD_SET (data->socket_id, &rset);

	char tmp[2] = {0};

	while (1) {
		nRet = select(maxfdp1, &rset, &wset, &eset, &tmOut);
		if (nRet == 0)
			break;
		recv(data->socket_id, tmp, 1, 0);
	}

	return 0;
}

static int __disconnect_tcp(socket_cli_t *s)
{
	s->is_connect = false;
	socket_data_t *data = (socket_data_t *)s->backend_data;
	if (INVALID_SOCKET != data->socket_id) {
		if (s->async)
			pthread_join(data->t_id, NULL);
#if defined(_WIN32)
		closesocket(data->socket_id);
#else
		close(data->socket_id);
#endif
	}
	data->socket_id = INVALID_SOCKET;

	return 0;
}

static void __destory_tcp(socket_cli_t *s)
{
	__disconnect_tcp(s);
	free(s->backend_data);
	free(s);

	return;
}

const socket_cli_backend_t _tcp_backend = {
	__connect_tcp,
	__read_tcp,
	__write_tcp,
	__clean_tcp,
	__disconnect_tcp,
	__destory_tcp,
};

socket_cli_t *socket_cli_new(int port, const char *addr, int async)
{
	if (-1 == __init()) { return NULL; }

	socket_cli_t *cli;
	socket_data_t *data;

	cli = (socket_cli_t *)malloc(sizeof(socket_cli_t));
	cli->backend = &_tcp_backend;

	data = (socket_data_t *)malloc(sizeof(socket_data_t));
	data->remote_addr.sin_port = htons(port);
	data->remote_addr.sin_family = AF_INET;

#if defined(_WIN32)
	data->remote_addr.sin_addr.s_addr = inet_addr(addr);
#else
	inet_pton(AF_INET, addr, &data->remote_addr.sin_addr.s_addr);
#endif

	cli->backend_data = data;
	cli->err_code = 0;
	cli->debug = 0;
	cli->is_connect = 0;
	cli->async = async;

	data->socket_id = INVALID_SOCKET;
	cli->is_connect = false;
	cli->debug = 0;

	return cli;
}

int socket_connect(socket_cli_t *s)
{
	assert(s);
	return s->backend->connect(s);
}

int socket_register_notify(socket_cli_t *s, notify_recv_cb recv_cb)
{
	assert(s);
	if (!s->is_connect) {
		printf("socket not connent, connect server first\n");
		return -1;
	}
	if (!s->async) {
		printf("sync mode neendn't register notify\n");
		return -1;
	}

	socket_data_t *data = (socket_data_t *)s->backend_data;
	data->recv_cb = recv_cb;
	pthread_create(&data->t_id, NULL, task_read, s);

	return 0;
}

int socket_read(socket_cli_t *s, void *buf, int len, int timeout)
{
	assert(s);
	return s->backend->read(s, buf, len, timeout);
}

int socket_write(socket_cli_t *s, void *buf, int len, int timeout)
{
	assert(s);
	return s->backend->write(s, buf, len, timeout);
}

int socket_clean(socket_cli_t *s)
{
	assert(s);
	return s->backend->clean(s);
}

int socket_disconnect(socket_cli_t *s)
{
	assert(s);
	return s->backend->disconnect(s);
}

void socket_destory(socket_cli_t *s)
{
	assert(s);
	return s->backend->destory(s);
}

bool socket_is_connect(socket_cli_t *s)
{
	assert(s);
	return s->is_connect;
}

void socket_set_debug(socket_cli_t *s, bool debug)
{
	assert(s);
	s->debug = debug;
}

/*
 * @describe 获取最新错误代码
 */
int socket_get_last_error(socket_cli_t *s)
{
	assert(s);
	return s->err_code;
}
