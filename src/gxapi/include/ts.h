#ifndef __TS_H_
#define __TS_H_

typedef enum TsSource_s {
	TS_SOURCE_1 = 0,
	TS_SOURCE_2,
	TS_SOURCE_3,
	TS_SOURCE_TOTAL,
} TsSource_e;

typedef enum DemuxId_s {
	DEMUX_ID_1 = 0,
	DEMUX_ID_2,
	DEMUX_ID_3,
	DEMUX_ID_4,
	DEMUX_ID_TOTAL,
} DemuxId_e;

typedef enum TsMode_s {
	TS_PARALLEL = 0,
	TS_SERIAL,
	TS_MODE_TOTAL,
} TsMode_e;

typedef struct DemuxConfigPara_s
{
	TsSource_e TsSource;
	DemuxId_e  DemuxId;
	TsMode_e   TsMode;
	int        DemuxDev;
	int        DemuxModule;
} DemuxConfigPara_t;

typedef struct TsFilterPara_s
{
	DemuxConfigPara_t DemuxPara;
	unsigned short  TsPID; //for slot,invilad id >= 0x1fff
	unsigned char   TsPackageLen; // 188 or 224
} TsFilterPara_t;

int ts_start(int pid);
int ts_read(char *pBuffer, unsigned int MaxBufferLen);
void ts_destroy_slot(void);

#endif /* __TS_H_ */
