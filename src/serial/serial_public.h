//
// Created by dell on 2018/6/25.
//

#ifndef __LIB_SERIAL_PUBLIC_H
#define __LIB_SERIAL_PUBLIC_H

#include <stdbool.h>

typedef struct _serial serial_t;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * @describe 创建读串口对象
 * @ret  null   创建失败
 *       非null 创建成功
 */
serial_t* serial_new(uint8_t index, int baud, char parity,
                     uint8_t data_bit, uint8_t stop_bit, uint8_t share_mode);

/*
 * @describe 打开串口
 * @ret  -1  打开失败
 *       0   打开成功
 */
int serial_open(serial_t *ctx);

/*
 * @describe 读取串口数据
 * @ret  -1  读取失败
 *       >=0 实际读取字节个数
 */
int serial_read(serial_t *ctx, unsigned char *buf, int len, int mtimeout);

/*
 * @describe 向串口写入数据
 * @ret  -1  写入失败
 *       >=0 实际写入字节个数
 */
int serial_write(serial_t *ctx, unsigned char *buf, int len, int mtimeout);

/*
 * @describe 清空串口缓存数据
 * @ret  -1  失败
 *       0   成功
 */
int serial_clean(serial_t *ctx);

/*
 * @describe 关闭串口
 * @ret  -1  失败
 *       0   成功
 */
int serial_close(serial_t *ctx);

/*
 * @describe 析构串口对象
 */
void serial_destory(serial_t *ctx);

/*
 * @describe 判断串口是否打开
 */
bool serial_is_open(serial_t *ctx);

/*
 * @describe 设置调试模式
 */
void serial_set_debug(serial_t *ctx, bool debug);

/*
 * @describe 获取最新错误代码
 */
int serial_get_last_error(serial_t *ctx);

void serial_lock(serial_t *ctx);

void serial_unlock(serial_t *ctx);

#ifdef __cplusplus
}
#endif

#endif //__LIB_SERIAL_PUBLIC_H
