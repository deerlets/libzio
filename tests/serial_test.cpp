//
// Created by dell on 2018/6/26.
//
#include <assert.h>
#include <unistd.h>
#include <gtest/gtest.h>
#include "serial/serial_public.h"

TEST(serial_t, basic)
{
	serial_t *t = serial_new(1, 9600, 'N', 8, 1, 1);
	serial_t *t1 = serial_new(1, 9600, 'N', 8, 1, 1);
	assert(t == t1);

	if (0 != serial_open(t))
		return;
	if (0 != serial_open(t1))
		return;

	unsigned char send[] = {'A', 'B', 'C', 'D', 'E'};
	unsigned char buf[11] = {0};
	printf("test\n");
	serial_clean(t);

	//write 5  read 10?
	int len = serial_write(t, send, 5, 100);
	printf("write len = %d\n", len);
	len = serial_read(t, buf, 10, 100);
	printf("read len = %d\n", len);
	for (int i = 0; i < len; ++i) {
		printf("0x%X ", buf[i]);
	}
	printf("\n\n\n");

	//write 10  read 5 read 2  clean read 5
	unsigned char send2[] = {'A', 'B', 'C', 'D', 'E', 'a', 'b', 'c', 'd', 'e'};
	unsigned char buf2[11] = {0};
	len = serial_write(t, send2, 10, 100);
	printf("write len = %d\n", len);
	len = serial_read(t, buf2, 5, 100);
	printf("read len = %d\n", len);
	for (int i = 0; i < len; ++i) {
		printf("0x%X ", buf2[i]);
	}
	printf("\n");

	len = serial_read(t, buf2, 2, 100);
	printf("read len = %d\n", len);
	for (int i = 0; i < len; ++i) {
		printf("0x%X ", buf2[i]);
	}
	printf("\n");

	serial_clean(t);
	len = serial_read(t, buf2, 5, 100);
	printf("read len = %d\n", len);
	for (int i = 0; i < len; ++i) {
		printf("0x%X ", buf[i]);
	}

	printf("\n");
	serial_close(t);
	serial_close(t1);

	printf("port open is %d \n", serial_is_open(t));
	serial_destory(t);
	serial_destory(t1);
}
