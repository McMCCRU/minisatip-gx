#include "ts.h"
#include "avapi.h"
#include "gxav_module_property.h"
#include "gxav_demux_propertytypes.h"
#include "gxav_event_type.h"

GxDemuxProperty_Slot sg_MuxSlot = { 0 };
GxDemuxProperty_Slot sg_Slot = { 0 };
GxDemuxProperty_Filter sg_Filter = { 0 };
GxDemuxProperty_Pcr pcr = { 0 };

static TsFilterPara_t sg_TsPara;
extern handle_t dev;
extern handle_t module;

static int sg_StartFlag = 0;

static int ts_init(int pid)
{
	//memcpy(&sg_TsPara, pTsFilterPara, sizeof(TsFilterPara_t));
	sg_TsPara.DemuxPara.DemuxDev = dev;
	sg_TsPara.DemuxPara.DemuxModule = module;

	sg_TsPara.DemuxPara.TsSource = TS_SOURCE_1; /* DEMUX_TS1 */
	sg_TsPara.DemuxPara.TsMode = TS_PARALLEL; /* DEMUX_PARALLEL */
	sg_TsPara.DemuxPara.DemuxId = DEMUX_ID_1; /* FRONTEND */

	sg_TsPara.TsPackageLen = 188;
	sg_TsPara.TsPID = pid; /* NTV+: 8 channel */

	return 0;
}

#define TS_DEMUX_IN_SIZE  (sg_TsPara.TsPackageLen * 1024 * 10)
#define TS_DEMUX_IN_GATE  (sg_TsPara.TsPackageLen * 30)

static int ts_setup_slot(void)
{
	int ret = 0;

	memset(&sg_MuxSlot, 0, sizeof(GxDemuxProperty_Slot));
	memset(&sg_Slot, 0, sizeof(GxDemuxProperty_Slot));
	memset(&sg_Filter, 0, sizeof(GxDemuxProperty_Filter));

	/* special slot */
	sg_MuxSlot.type = DEMUX_SLOT_MUXTS;
	sg_MuxSlot.flags = (DMX_TSOUT_EN | DMX_CRC_DISABLE | DMX_REPEAT_MODE);
	ret = GxAVGetProperty(sg_TsPara.DemuxPara.DemuxDev, sg_TsPara.DemuxPara.DemuxModule, GxDemuxPropertyID_SlotAlloc,
					&sg_MuxSlot, sizeof(GxDemuxProperty_Slot));
	if (ret < 0)
	{
		printf("TS: GxDemuxPropertyID_SlotAlloc1 Problem...\n");
		return -1;
	}

	/* special filter */
	sg_Filter.slot_id = sg_MuxSlot.slot_id;
	sg_Filter.sw_buffer_size = TS_DEMUX_IN_SIZE;
	sg_Filter.hw_buffer_size = TS_DEMUX_IN_SIZE;
	sg_Filter.almost_full_gate = TS_DEMUX_IN_GATE;

	ret = GxAVGetProperty(sg_TsPara.DemuxPara.DemuxDev, sg_TsPara.DemuxPara.DemuxModule, GxDemuxPropertyID_FilterAlloc,
					&sg_Filter, sizeof(GxDemuxProperty_Filter));
	if (ret < 0)
	{
		printf("TS: GxDemuxPropertyID_FilterAlloc Problem...\n");
		return -1;
	}

	/* normal slot */
	sg_Slot.type = DEMUX_SLOT_TS;
	sg_Slot.flags = (DMX_REPEAT_MODE | DMX_TSOUT_EN | DMX_CRC_DISABLE);
	sg_Slot.ts_out_pin = sg_MuxSlot.slot_id;
	sg_Slot.pid = sg_TsPara.TsPID; // Video PID
	ret = GxAVGetProperty(sg_TsPara.DemuxPara.DemuxDev, sg_TsPara.DemuxPara.DemuxModule, GxDemuxPropertyID_SlotAlloc,
					&sg_Slot, sizeof(GxDemuxProperty_Slot));
	if (ret < 0)
	{
		printf("TS: GxDemuxPropertyID_SlotAlloc2 Problem...\n");
		return -1;
	}
	/* config slot */
	ret = GxAVSetProperty(sg_TsPara.DemuxPara.DemuxDev,sg_TsPara.DemuxPara.DemuxModule, GxDemuxPropertyID_SlotConfig,
					&sg_Slot, sizeof(GxDemuxProperty_Slot));
	if (ret < 0)
	{
		printf("TS: GxDemuxPropertyID_SlotConfig Problem...\n");
		return -1;
	}
	/* enable slot */
	ret = GxAVSetProperty(sg_TsPara.DemuxPara.DemuxDev,sg_TsPara.DemuxPara.DemuxModule, GxDemuxPropertyID_SlotEnable,
					&sg_Slot, sizeof(GxDemuxProperty_Slot));
	if (ret < 0)
	{
		printf("TS: GxDemuxPropertyID_SlotEnable Problem...\n");
		return -1;
	}

	return 0;
}

static int check_ts_lock(void)
{
	int ret = -1;
	GxDemuxProperty_TSLockQuery ts_lock_status = { TS_SYNC_UNLOCKED };

	ret = GxAVGetProperty(sg_TsPara.DemuxPara.DemuxDev, sg_TsPara.DemuxPara.DemuxModule, GxDemuxPropertyID_TSLockQuery,
					&ts_lock_status, sizeof(GxDemuxProperty_TSLockQuery));
	if(ret < 0)
	{
		printf("TS: GxDemuxPropertyID_TSLockQuery Problem...\n");
		return -1;
	}
	return ((ts_lock_status.ts_lock == TS_SYNC_LOCKED) ? 1 : 0);
}

void ts_destroy_slot(void)
{
	if(sg_Slot.slot_id > 0)
	{
		/* disable slot */
		GxAVSetProperty(sg_TsPara.DemuxPara.DemuxDev, sg_TsPara.DemuxPara.DemuxModule, GxDemuxPropertyID_SlotDisable,
					(void*)&sg_Slot, sizeof(GxDemuxProperty_Slot));

		/* special filter */
		GxAVSetProperty(sg_TsPara.DemuxPara.DemuxDev, sg_TsPara.DemuxPara.DemuxModule, GxDemuxPropertyID_FilterFree,
					(void*)&sg_Filter, sizeof(GxDemuxProperty_Filter));

		GxAVSetProperty(sg_TsPara.DemuxPara.DemuxDev, sg_TsPara.DemuxPara.DemuxModule, GxDemuxPropertyID_SlotFree,
					(void*)&sg_Slot, sizeof(GxDemuxProperty_Slot));

		GxAVSetProperty(sg_TsPara.DemuxPara.DemuxDev, sg_TsPara.DemuxPara.DemuxModule, GxDemuxPropertyID_SlotFree,
					(void*)&sg_MuxSlot, sizeof(GxDemuxProperty_Slot));

		memset(&sg_MuxSlot, 0, sizeof(GxDemuxProperty_Slot));
		memset(&sg_Slot, 0, sizeof(GxDemuxProperty_Slot));
		memset(&sg_Filter, 0, sizeof(GxDemuxProperty_Filter));

		sg_Slot.slot_id = -1;
		sg_MuxSlot.slot_id = -1;
	}
}

int ts_start(int pid)
{
	int ret = -1;

	ts_init(pid);

	ret = ts_setup_slot();
	if(ret < 0)
		return -1;

	ret = check_ts_lock();
	if(!ret) {
		printf("TS: TS LOCK Problem...\n");
		return -1;
	}
	return 0;
}

int ts_read(char *pBuffer, unsigned int MaxBufferLen)
{
	int ret = 0;
	unsigned int EventRet = 0;
	GxDemuxProperty_FilterRead DmxFilterRead;

	ret = GxAVWaitEvents(sg_TsPara.DemuxPara.DemuxDev, sg_TsPara.DemuxPara.DemuxModule, EVENT_DEMUX0_FILTRATE_TS_END, 1000000, &EventRet);
	if(ret < 0) {
		printf("TS: GxAVWaitEvents Problem...\n");
		return -1;
	}

	DmxFilterRead.filter_id = sg_Filter.filter_id;
	DmxFilterRead.buffer    = pBuffer;
	DmxFilterRead.max_size  = MaxBufferLen;

	ret = GxAVGetProperty(sg_TsPara.DemuxPara.DemuxDev, sg_TsPara.DemuxPara.DemuxModule, GxDemuxPropertyID_FilterRead,
					(void*)&DmxFilterRead, sizeof(GxDemuxProperty_FilterRead));
	if(ret < 0)
	{
		printf("TS read, failed...\n");
		return -1;
	}

	if(DmxFilterRead.read_size < 0)
		DmxFilterRead.read_size = 0;

	return DmxFilterRead.read_size;
}
