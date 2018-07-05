//
// Created by dell on 2018/7/2.
//

#include <assert.h>
#include <unistd.h>
#include <gtest/gtest.h>
#include "socket/socket_public.h"

void read_from_server(void *buf, int lenth)
{
	printf("read from server: [%s][%d]\n", (char *)buf, lenth);
}

TEST(socket_cli_t, basic)
{
	//同步socket测试
	socket_cli_t *t1 = socket_cli_new(8808, "172.30.141.193", 0);
	socket_set_debug(t1, 1);
	socket_connect(t1);
	char send_buf[10] = "123456789";
	socket_write(t1, send_buf, 10, 1000);
	char read_buf[10] = {0};
	socket_read(t1, read_buf, 10, 10000);
	socket_disconnect(t1);
	socket_destory(t1);

	//异步通过回调函数读入socket测试
	socket_cli_t *t2 = socket_cli_new(8808, "172.30.141.193", 1);
	socket_set_debug(t2, 1);
	socket_connect(t2);
	socket_register_notify(t2, read_from_server);
	char send_buf2[10] = "123456789";
	socket_write(t2, send_buf2, 10, 1000);

	int count = 100;
	while (count > 0) {
		sleep(1);
		count--;
	}
	socket_disconnect(t2);
	socket_destory(t2);

	return;
}
