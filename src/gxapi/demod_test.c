#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <sys/param.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include <frontend.h>
//#include <linux/dvb/frontend.h>

#include <avapi.h>
#include <gxav_module_property.h>
#include <gxav_demux_propertytypes.h>
#include <ts.h>

typedef enum _FRONTEND_MODE
{
	DVBS2_NORMAL,
	DVBS2_BLIND,
	DVBT_AUTO_MODE,
	DVBT_NORMAL,
	DVBT2_BASE,
	DVBT2_LITE,
	DVBT2_BASE_LITE,
	DVBC_J83A,
	DVBC_J83B,
	DTMB_C,
	DTMB,
} FRONTEND_MODE;

handle_t dev = -1, module = -1;

static int demux_config(void)
{
	int ret = 0;

	GxDemuxProperty_ConfigDemux demux;

	dev = GxAVCreateDevice(0);
	if(dev < 0)
	{
		printf("ERR_DEMUX: Not open device....\n");
		return -1;
	}

	printf("Chip detected: 0x%04X\n", GxChipDetect(dev));

	module = GxAVOpenModule(dev, GXAV_MOD_DEMUX, 0);
	if(module < 0)
	{
		printf("ERR_DEMUX: Not start module....\n");
		GxAVDestroyDevice(dev);
		return -1;
	}

	// demux config
	demux.source = DEMUX_TS1;
	demux.ts_select = FRONTEND;
	demux.stream_mode = DEMUX_PARALLEL;
	demux.time_gate = 0xf;
	demux.byt_cnt_err_gate = 0x03;
	demux.sync_loss_gate = 0x03;
	demux.sync_lock_gate = 0x03;

	ret = GxAVSetProperty(dev, module, GxDemuxPropertyID_Config, &demux, sizeof(GxDemuxProperty_ConfigDemux));
	if(ret < 0)
	{
		printf("ERR_DEMUX: Not set property module....\n");
		GxAVCloseModule(dev, module);
		GxAVDestroyDevice(dev);
		return -1;
	}

	return 0;
}

static int set_voltage(int fd, fe_sec_voltage_t volt, fe_sec_tone_mode_t tone)
{
	if(ioctl(fd, FE_SET_VOLTAGE, volt) < 0)
	{
		printf("IOCTL: Problem FE_SET_VOLTAGE....\n");
		return -1;
	}

	if(ioctl(fd, FE_SET_TONE, tone) < 0)
		printf("IOCTL: Problem FE_SET_TONE....\n");

	printf("Set voltage %dV, tone is %s\n", (volt == SEC_VOLTAGE_OFF) ? 0 : (volt == SEC_VOLTAGE_13) ? 13 : 18, (tone == SEC_TONE_OFF) ? "OFF" : "ON");

	return 0;
}

static int do_tune(int fd)
{
	int res = -1;

	struct dvb_frontend_parameters tuneto;
	struct dvb_frontend_event ev;

	if(ioctl(fd, FE_SET_FRONTEND_TUNE_MODE, DVBS2_NORMAL) < 0) {
		printf("IOCTL: Problem FE_SET_FRONTEND_TUNE_MODE....\n");
		return -1;
	}

	if(set_voltage(fd, SEC_VOLTAGE_13, SEC_TONE_OFF)) /* polar = R */
		return -1;

	usleep(100000);

	tuneto.frequency = (12245000 - 10750000); /* 1495000, LNB cir. */
	tuneto.inversion = INVERSION_AUTO;
	tuneto.u.qpsk.symbol_rate = 27500000;
	tuneto.u.qpsk.fec_inner = FEC_AUTO;

	/* discard stale QPSK events */
	while (1) {
		if(ioctl(fd, FE_GET_EVENT, &ev) < 0)
			break;
	}

	if((res = ioctl(fd, FE_SET_FRONTEND, &tuneto)) < 0)
	{
		printf("IOCTL: Problem FE_SET_FRONTEND....%d\n", res);
		return -1;
	}

	return 0;
}

static int check_tp_lock(int fd)
{
	fe_status_t status;
	int i, ret = 0;
	unsigned short snr, signal;
	unsigned int ber, uncorrected_blocks;

	for(i = 0; i < 3; i++) {
		if(ioctl(fd, FE_READ_STATUS, &status) < 0)
		{
			printf("IOCTL: Problem FE_READ_STATUS....\n");
			ret = -1;
			break;
		}

		if(ioctl(fd, FE_READ_SIGNAL_STRENGTH, &signal) == -1)
			signal = -2;
		if(ioctl(fd, FE_READ_SNR, &snr) == -1)
			snr = -2;
		if(ioctl(fd, FE_READ_BER, &ber) == -1)
			ber = -2;
		if(ioctl(fd, FE_READ_UNCORRECTED_BLOCKS, &uncorrected_blocks) == -1)
			uncorrected_blocks = -2;

		printf ("status %02x | signal %3u%% | snr %3u%% | ber %d | unc %d | ",
			status, (signal * 100) / 0xffff, (snr * 100) / 0xffff, ber, uncorrected_blocks);

		if(status & FE_HAS_LOCK) {
			printf("Good: TP is LOCKED!!!!\n");
		} else {
			printf("Bad: TP is NOT LOCKED!!!!\n");
		}
		usleep(1000000);
	}
	return ret;
}

static int fe_get_info(int fd)
{
	struct dvb_frontend_info info;

	if(ioctl(fd, FE_GET_INFO, &info) < 0)
	{
		printf("IOCTL: Problem FE_GET_INFO....\n");
		return -1;
	}

	printf("Info:\n");
	printf("Name: %s\n", info.name);
	printf("Type: %d\n", (int)info.type);
	printf("Frequency MIN: %u\n", info.frequency_min);
	printf("Frequency MAX: %u\n", info.frequency_max);
	printf("Frequency stepsize: %u\n", info.frequency_stepsize);
	printf("Frequency tolerance: %u\n", info.frequency_tolerance);
	printf("Symbol Rate MIN: %u\n", info.symbol_rate_min);
	printf("Symbol Rate MAX: %u\n", info.symbol_rate_max);
	printf("Symbol Rate tolerance: %u\n", info.symbol_rate_tolerance);
	printf("Notifier delay: %u\n", info.notifier_delay);
	printf("FE CAPS: 0x%08X\n", (unsigned int)info.caps);

	return 0;
}

#ifndef O_CLOEXEC
# define O_CLOEXEC 02000000
#endif

int main(void)
{
	int i, s, len, fd, tsfd;
	char *tsbuf;

	len = 188 * 100 * 3;

	fd = open("/dev/dvb0.frontend0", O_RDWR | O_NONBLOCK | O_CLOEXEC);
	if (fd < 0) {
		printf("Not open frontend interface!!!\n");
		return -1;
	}

	if(demux_config()) {
		close(fd);
		return -1;
	}

	if(fe_get_info(fd)) {
		close(fd);
		GxAVCloseModule(dev, module);
		GxAVDestroyDevice(dev);
		return -1;
	}

	if(do_tune(fd)) {
		close(fd);
		GxAVCloseModule(dev, module);
		GxAVDestroyDevice(dev);
		return -1;
	}

	if(check_tp_lock(fd)) {
		GxAVCloseModule(dev, module);
		GxAVDestroyDevice(dev);
		close(fd);
		return -1;
	}

	if(ts_start(0xc9)) { // Video & PCR Pid - 8 channel NTV+
		ts_destroy_slot();
		GxAVCloseModule(dev, module);
		GxAVDestroyDevice(dev);
		close(fd);
		return -1;
	}

	tsfd = open("/media/sda1/test.ts", O_RDWR | O_NONBLOCK);
	if (tsfd < 0) {
		printf("Not open file stream!!!\n");
		goto the_end;
	}

	tsbuf = calloc(1, len);

	for (i = 0; i < 50000; i++) {
		s = ts_read(tsbuf, len);
		//if(!s)
		//	continue;
		if(s >= 188) {
			write(tsfd, tsbuf, s);
		} else
			break;
	}

	free(tsbuf);

the_end:
	close(tsfd);
	ts_destroy_slot();
	GxAVCloseModule(dev, module);
	GxAVDestroyDevice(dev);
	close(fd);
	return 0;
}
