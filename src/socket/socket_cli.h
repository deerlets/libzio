//
// Created by dell on 2018/7/2.
//

#ifndef __LIB_SOCKET_CLI_H
#define __LIB_SOCKET_CLI_H

#include <pthread.h>
#include "socket_public.h"

#if defined(_WIN32)

#include <winsock2.h>
#include <ws2tcpip.h>

#else
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/prctl.h>
#include <errno.h>
#include <stdio.h>

#define INVALID_SOCKET -1

#endif

typedef struct _socket_data socket_data_t;

typedef struct _socket_cli_backend {
	int (*connect)(socket_cli_t *s);
	int (*read)(socket_cli_t *s, void *buf, int len, int timeout);
	int (*read_all)(socket_cli_t *s, void *buf, int len, int timeout);
	int (*write)(socket_cli_t *s, void *buf, int len, int timeout);
	int (*clean)(socket_cli_t *s);
	int (*disconnect)(socket_cli_t *s);
	void (*destory)(socket_cli_t *s);
} socket_cli_backend_t;

struct _socket_cli {
	int debug;
	bool is_connect;
	int err_code;
	int async;                            // 0 同步读  1 异步通过回调函数读
	int con_timeout;                      // 连接超时

	const socket_cli_backend_t *backend;
	void *backend_data;
};

struct _socket_data {
	notify_recv_cb recv_cb;
	pthread_t t_id;
	int socket_id;
	struct sockaddr_in remote_addr;
};

#endif //__LIB_SOCKET_CLI_H
