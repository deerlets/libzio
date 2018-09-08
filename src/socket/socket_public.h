//
// Created by dell on 2018/7/2.
//

#ifndef __LIB_SOCKET_PUBLIC_H
#define __LIB_SOCKET_PUBLIC_H

#include <stdbool.h>

#define PROTO_TCP 0
#define PROTO_UDP 1

typedef struct _socket_cli socket_cli_t;

typedef void (*notify_recv_cb)(void *buf, int len);

#ifdef __cplusplus
extern "C" {
#endif

/*
 * @describe    创建客户端套接字
 * @ret  null   创建失败
 *       非null 创建成功
 */
socket_cli_t *socket_cli_new(int port, const char *addr, int async, int con_timeout);

/*
 * @describe    连接服务器
 * @ret  0      连接成功
 *       -1     连接失败
 */
int socket_connect(socket_cli_t *s);

/*
 * @describe    注册recv回调函数 async 模式使用，connect之后调用
 * @ret  0      成功
 *       -1     失败
 */
int socket_register_notify(socket_cli_t *s, notify_recv_cb recv_cb);

/*
 * @describe    读取数据
 * @ret  >0     实际读取字节个数
 *       -1     失败
 */
int socket_read(socket_cli_t *s, void *buf, int len, int timeout);

/*
 * @describe    读取数据
 * @ret  >0     实际写入字节个数
 *       -1     失败
 */
int socket_write(socket_cli_t *s, void *buf, int len, int timeout);

/*
 * @describe    清空缓存
 * @ret  0      成功
 *       -1     失败
 */
int socket_clean(socket_cli_t *s);

/*
 * @describe    断开连接
 * @ret  0      成功
 *       -1     失败
 */
int socket_disconnect(socket_cli_t *s);

/*
 * @describe    释放对象
 */
void socket_destory(socket_cli_t *s);

/*
 * @describe 判断是否连接
 */
bool socket_is_connect(socket_cli_t *s);

/*
 * @describe 设置调试模式
 */
void socket_set_debug(socket_cli_t *s, bool debug);

/*
 * @describe 获取最新错误代码
 */
int socket_get_last_error(socket_cli_t *s);

#ifdef __cplusplus
}
#endif

#endif //__LIB_SOCKET_PUBLIC_H
