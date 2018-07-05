//
// Created by dell on 2018/6/25.
//

#if defined(_WIN32)

#include "serial_win.h"
#include <stdio.h>

int win32_serial_open(serial_t *ctx)
{
	if (ctx->is_open)
		return 0;

	DCB dcb;
	serial_rtu_t *ctx_rtu = (serial_rtu_t *)ctx->backend_data;

	if (ctx->debug) {
		printf("Opening %s at %d bauds (%c, %d, %d)\n",
		       ctx_rtu->device, ctx_rtu->baud, ctx_rtu->parity,
		       ctx_rtu->data_bit, ctx_rtu->stop_bit);
	}

	ctx_rtu->fd = CreateFileA(ctx_rtu->device,
	                          GENERIC_READ | GENERIC_WRITE,
	                          0,
	                          NULL,
	                          OPEN_EXISTING,
	                          0,
	                          NULL);

	/* Error checking */
	if (ctx_rtu->fd == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "ERROR Can't open the device %s (%s)\n",
		        ctx_rtu->device, strerror(errno));
		ctx->err_code = GetLastError();

		return -1;
	}

	/* Save params */
	ctx_rtu->old_dcb.DCBlength = sizeof(DCB);
	if (!GetCommState(ctx_rtu->fd, &ctx_rtu->old_dcb)) {
		fprintf(stderr, "ERROR Error getting configuration (LastError %d)\n",
		        (int)GetLastError());
		CloseHandle(ctx_rtu->fd);
		ctx_rtu->fd = INVALID_HANDLE_VALUE;
		ctx->err_code = GetLastError();

		return -1;
	}

	/* Build new configuration (starting from current settings) */
	dcb = ctx_rtu->old_dcb;

	/* Speed setting */
	switch (ctx_rtu->baud) {
	case 110:
		dcb.BaudRate = CBR_110;
		break;
	case 300:
		dcb.BaudRate = CBR_300;
		break;
	case 600:
		dcb.BaudRate = CBR_600;
		break;
	case 1200:
		dcb.BaudRate = CBR_1200;
		break;
	case 2400:
		dcb.BaudRate = CBR_2400;
		break;
	case 4800:
		dcb.BaudRate = CBR_4800;
		break;
	case 9600:
		dcb.BaudRate = CBR_9600;
		break;
	case 19200:
		dcb.BaudRate = CBR_19200;
		break;
	case 38400:
		dcb.BaudRate = CBR_38400;
		break;
	case 57600:
		dcb.BaudRate = CBR_57600;
		break;
	case 115200:
		dcb.BaudRate = CBR_115200;
		break;
	default:
		dcb.BaudRate = CBR_9600;
		printf("WARNING Unknown baud rate %d for %s (B9600 used)\n",
		       ctx_rtu->baud, ctx_rtu->device);
	}

	/* Data bits */
	switch (ctx_rtu->data_bit) {
	case 5:
		dcb.ByteSize = 5;
		break;
	case 6:
		dcb.ByteSize = 6;
		break;
	case 7:
		dcb.ByteSize = 7;
		break;
	case 8:
	default:
		dcb.ByteSize = 8;
		break;
	}

	/* Stop bits */
	if (ctx_rtu->stop_bit == 1)
		dcb.StopBits = ONESTOPBIT;
	else /* 2 */
		dcb.StopBits = TWOSTOPBITS;

	/* Parity */
	if (ctx_rtu->parity == 'N') {
		dcb.Parity = NOPARITY;
		dcb.fParity = FALSE;
	} else if (ctx_rtu->parity == 'E') {
		dcb.Parity = EVENPARITY;
		dcb.fParity = TRUE;
	} else {
		/* odd */
		dcb.Parity = ODDPARITY;
		dcb.fParity = TRUE;
	}

	/* Hardware handshaking left as default settings retrieved */

	/* No software handshaking */
	dcb.fTXContinueOnXoff = TRUE;
	dcb.fOutX = FALSE;
	dcb.fInX = FALSE;

	/* Binary mode (it's the only supported on Windows anyway) */
	dcb.fBinary = TRUE;

	/* Don't want errors to be blocking */
	dcb.fAbortOnError = FALSE;

	/* TODO: any other flags!? */

	/* Setup port */
	if (!SetCommState(ctx_rtu->fd, &dcb)) {
		ctx->err_code = GetLastError();
		fprintf(stderr,
		        "ERROR Error setting new configuration (LastError %d)\n",
		        ctx->err_code);
		win32_serial_close(ctx);

		return -1;
	}

#if HAVE_DECL_TIOCSRS485
	/* The RS232 mode has been set by default */
	ctx_rtu->serial_mode = RS232;
#endif

	ctx->is_open = true;

	return 0;
}

int win32_serial_set_timeout(serial_t *ctx, int mtimeout)
{
	COMMTIMEOUTS ctimeout;
	serial_rtu_t *ctx_rtu = (serial_rtu_t *)ctx->backend_data;

	ctimeout.ReadIntervalTimeout = mtimeout;
	ctimeout.ReadTotalTimeoutMultiplier = 1;
	ctimeout.ReadTotalTimeoutConstant = mtimeout;
	ctimeout.WriteTotalTimeoutMultiplier = 1;
	ctimeout.WriteTotalTimeoutConstant = mtimeout;

	if (!SetCommTimeouts(ctx_rtu->fd, &ctimeout)) {
		ctx->err_code = GetLastError();
		return -1;
	}

	return 0;
}

/*
 * @ret -1  读入失败
 *      >=0 实际读入字节数
 */
int
win32_serial_read(serial_t *ctx, unsigned char *buf, int len, int mtimeout)
{
	if (!ctx->is_open)
		return -1;

	bool ret;
	unsigned long retlen;

	if (mtimeout >= 0) win32_serial_set_timeout(ctx, mtimeout);

	serial_rtu_t *ctx_rtu = (serial_rtu_t *)ctx->backend_data;

	ret = ReadFile(
		ctx_rtu->fd,           // handle of file to read
		buf,                   // pointer to buffer that receives data
		len,                   // number of bytes to read
		&retlen,               // pointer to number of bytes read
		NULL                   // pointer to structure for data
	);

	if (!ret) {
		ctx->err_code = GetLastError();
		return -1;
	}

	if (ctx->debug) printf("win32_serial_read retlen=%u\n", retlen);

	return (int)retlen;
}

int
win32_serial_write(serial_t *ctx, unsigned char *buf, int len, int mtimeout)
{
	if (!ctx->is_open)
		return -1;

	bool ret;
	unsigned long retlen;
	serial_rtu_t *ctx_rtu = (serial_rtu_t *)ctx->backend_data;

	if (ctx->debug) {
		for (int i = 0; i < len; i++) printf(" %d", buf[i]);
		printf("\n");
	}
	retlen = len;
	ret = WriteFile(
		ctx_rtu->fd,           // handle to file to write to
		buf,                   // pointer to data to write to file
		len,                   // number of bytes to write
		&retlen,               // pointer to number of bytes written
		NULL                   // pointer to structure for overlapped I/O
	);

	if (ret) return (int)retlen;

	ctx->err_code = GetLastError();

	return -1;
}

int win32_serial_clean_buffer(serial_t *ctx)
{
	serial_rtu_t *ctx_rtu = (serial_rtu_t *)ctx->backend_data;

	if (!PurgeComm(ctx_rtu->fd, PURGE_TXCLEAR | PURGE_RXCLEAR)) {
		ctx->err_code = GetLastError();
		return -1;
	}

	return 0;
}

int win32_serial_close(serial_t *ctx)
{
	serial_rtu_t *ctx_rtu = (serial_rtu_t *)ctx->backend_data;
	if (ctx->is_open || INVALID_HANDLE_VALUE != ctx_rtu->fd) {
		/* Revert settings */
		if (!SetCommState(ctx_rtu->fd, &ctx_rtu->old_dcb)) {
			ctx->err_code = GetLastError();
			fprintf(stderr,
			        "ERROR Couldn't revert to configuration (LastError %d)\n",
			        ctx->err_code);
			return -1;
		}

		if (!CloseHandle(ctx_rtu->fd)) {
			ctx->err_code = GetLastError();
			fprintf(stderr, "ERROR Error while closing handle (LastError %d)\n",
			        ctx->err_code);
			return -1;
		}

		ctx_rtu->fd = INVALID_HANDLE_VALUE;
		ctx->is_open = false;
	}

	return 0;
}

void win32_serial_destory(serial_t *ctx)
{
	win32_serial_close(ctx);
	free(ctx->backend_data);
	free(ctx);
	ctx = NULL;
}

#endif
