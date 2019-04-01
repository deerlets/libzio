//
// Created by dell on 2018/6/25.
//

#include "serial_unix.h"

#if (defined(__unix__) || defined(unix))

int unix_serial_open(serial_t *ctx)
{
	if (ctx->is_open)
		return 0;

	serial_rtu_t *ctx_rtu = (serial_rtu_t *)ctx->backend_data;
	struct termios tios;
	speed_t speed;

	ctx_rtu->s = open(ctx_rtu->device, O_RDWR | O_NOCTTY | O_NDELAY | O_EXCL);
	if (ctx_rtu->s == -1) {
		fprintf(stderr, "ERROR Can't open the device %s (%s)\n",
		        ctx_rtu->device, strerror(errno));
		ctx->err_code = errno;

		return -1;
	}

	/* Save */
	tcgetattr(ctx_rtu->s, &(ctx_rtu->old_tios));

	memset(&tios, 0, sizeof(struct termios));

	/*
	 * C_ISPEED     Input baud (new interface)
	 * C_OSPEED     Output baud (new interface)
	 */
	switch (ctx_rtu->baud) {
	case 110:
		speed = B110;
		break;
	case 300:
		speed = B300;
		break;
	case 600:
		speed = B600;
		break;
	case 1200:
		speed = B1200;
		break;
	case 2400:
		speed = B2400;
		break;
	case 4800:
		speed = B4800;
		break;
	case 9600:
		speed = B9600;
		break;
	case 19200:
		speed = B19200;
		break;
	case 38400:
		speed = B38400;
		break;
	case 57600:
		speed = B57600;
		break;
	case 115200:
		speed = B115200;
		break;
	default:
		speed = B9600;
		if (ctx->debug) {
			fprintf(stderr,
			        "WARNING Unknown baud rate %d for %s (B9600 used)\n",
			        ctx_rtu->baud, ctx_rtu->device);
		}
	}

	/* Set the baud rate */
	if ((cfsetispeed(&tios, speed) < 0) ||
	    (cfsetospeed(&tios, speed) < 0)) {
		ctx->err_code = errno;
		close(ctx_rtu->s);
		ctx_rtu->s = -1;

		return -1;
	}

	/*
	 * C_CFLAG      Control options
	 *  CLOCAL       Local line - do not change "owner" of port
	 *  CREAD        Enable receiver
	 */
	tios.c_cflag |= (CREAD | CLOCAL);
	/* CSIZE, HUPCL, CRTSCTS (hardware flow control) */

	/*
	 * Set data bits (5, 6, 7, 8 bits)
	 * CSIZE        Bit mask for data bits
	 */
	tios.c_cflag &= ~CSIZE;
	switch (ctx_rtu->data_bit) {
	case 5:
		tios.c_cflag |= CS5;
		break;
	case 6:
		tios.c_cflag |= CS6;
		break;
	case 7:
		tios.c_cflag |= CS7;
		break;
	case 8:
	default:
		tios.c_cflag |= CS8;
		break;
	}

	/* Stop bit (1 or 2) */
	if (ctx_rtu->stop_bit == 1)
		tios.c_cflag &= ~CSTOPB;
	else /* 2 */
		tios.c_cflag |= CSTOPB;

	/*
	 * PARENB       Enable parity bit
	 * PARODD       Use odd parity instead of even
	 */
	if (ctx_rtu->parity == 'N') {
		/* None */
		tios.c_cflag &= ~PARENB;
	} else if (ctx_rtu->parity == 'E') {
		/* Even */
		tios.c_cflag |= PARENB;
		tios.c_cflag &= ~PARODD;
	} else {
		/* Odd */
		tios.c_cflag |= PARENB;
		tios.c_cflag |= PARODD;
	}

	/* Raw input */
	tios.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	if (ctx_rtu->parity == 'N') {
		/* None */
		tios.c_iflag &= ~INPCK;
	} else {
		tios.c_iflag |= INPCK;
	}

	/* Software flow control is disabled */
	tios.c_iflag &= ~(IXON | IXOFF | IXANY);

	/*
	 * C_OFLAG      Output options
	 * OPOST        Postprocess output (not set = raw output)
	 * ONLCR        Map NL to CR-NL
	 *
	 * ONCLR ant others needs OPOST to be enabled
	 */

	/* Raw ouput */
	tios.c_oflag &= ~OPOST;

	/* Unused because we use open with the NDELAY option */
	tios.c_cc[VMIN] = 0;
	tios.c_cc[VTIME] = 0;

	if (tcsetattr(ctx_rtu->s, TCSANOW, &tios) < 0) {
		ctx->err_code = errno;
		fprintf(stderr, "ERROR tcsetattr error (%s) (%s)\n",
		        ctx_rtu->device, strerror(errno));
		close(ctx_rtu->s);
		ctx_rtu->s = -1;
		return -1;
	}

	ctx->is_open = true;

#if HAVE_DECL_TIOCSRS485
	/* The RS232 mode has been set by default */
	ctx_rtu->serial_mode = RS232;
#endif

	return 0;
}

int unix_serial_set_timeout(serial_t *ctx, int mtimeout)
{
	struct timeval timout;
	fd_set wset, rset, eset;
	int ret, maxfdp1;
	serial_rtu_t *ctx_rtu = (serial_rtu_t *)ctx->backend_data;

	if (mtimeout <= 0) return 0;
	/* setup sockets to read */
	maxfdp1 = ctx_rtu->s + 1;
	FD_ZERO(&rset);
	FD_SET (ctx_rtu->s, &rset);
	FD_ZERO(&wset);
	FD_ZERO(&eset);
	timout.tv_sec = mtimeout / 1000;
	timout.tv_usec = (mtimeout % 1000) * 1000;

	ret = select(maxfdp1, &rset, &wset, &eset, &timout);

	if (ret == 0) return -1; /* timeout */

	return 0;
}

int unix_serial_read(serial_t *ctx, unsigned char *buf, int len, int mtimeout)
{
	if (!ctx->is_open)
		return -1;

	serial_rtu_t *ctx_rtu = (serial_rtu_t *)ctx->backend_data;

	if (ctx_rtu->s == -1) {
		return -1;
	}

	int read_len = 0;
	while(read_len < len) {
		if (mtimeout >= 0)
			if (-1 == unix_serial_set_timeout(ctx, mtimeout)) break;
		unsigned char temp[1024] = {0};
		int temp_len = len - read_len < 1024 ? len - read_len : 1024;
		int cur_len = read(ctx_rtu->s, temp, temp_len);

		if (cur_len <= 0) return read_len;
		memcpy(buf[read_len], temp, cur_len);
		read_len += cur_len;
	}

	if (read_len <= 0) {
		ctx->err_code = errno;
		return -1;
	}

	return read_len;
}

int unix_serial_write(serial_t *ctx, unsigned char *buf, int len, int mtimeout)
{
	if (!ctx->is_open)
		return -1;

	int ret;
	serial_rtu_t *ctx_rtu = (serial_rtu_t *)ctx->backend_data;

	if (ctx_rtu->s == -1) {
		printf("unix_serial_write s = -1\n");
		ctx->err_code = errno;

		return -1;
	}
	if (ctx->debug) {
		for (int i = 0; i < len; i++) printf(" %d", (int)buf[i]);
		printf("\n");
	}

	ret = write(ctx_rtu->s, buf, len);

	if (-1 == ret) ctx->err_code = errno;

	return ret;
}

int unix_serial_clean_buffer(serial_t *ctx)
{
	serial_rtu_t *ctx_rtu = (serial_rtu_t *)ctx->backend_data;
	tcflush(ctx_rtu->s, TCIOFLUSH);
	return 0;
}

int unix_serial_close(serial_t *ctx)
{
	serial_rtu_t *ctx_rtu = (serial_rtu_t *)ctx->backend_data;
	if (ctx_rtu->s == -1) return -1;

	if (tcsetattr(ctx_rtu->s, TCSANOW, &ctx_rtu->old_tios) < 0) {
		ctx->err_code = errno;
		return -1;
	}
	close(ctx_rtu->s);

	ctx->is_open = false;
	ctx_rtu->s = -1;

	return 0;
}

void unix_serial_destory(serial_t *ctx)
{
	unix_serial_close(ctx);
	free(ctx->backend_data);
	free(ctx);

	return;
}

#endif //_UNIX
