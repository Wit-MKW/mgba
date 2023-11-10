#include <mgba/internal/gba/sio/mobile.h>

#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/io.h>

mLOG_DECLARE_CATEGORY(GBA_MOBILE);
mLOG_DEFINE_CATEGORY(GBA_MOBILE, "Mobile Adapter (GBA)", "gba.mobile");

static void debug_log(void* user, const char* line) {
	UNUSED(user);
	mLOG(GBA_MOBILE, DEBUG, "%s", line);
}

static void time_latch(void* user, unsigned timer) {
	struct GBASIOMobileAdapter* adapter = ((struct MobileAdapterGB*) user)->p;

	adapter->timeLatch[timer] = mTimingCurrentTime(&adapter->d.p->p->timing);
}

static bool time_check_ms(void* user, unsigned timer, unsigned ms) {
	struct GBASIOMobileAdapter* adapter = ((struct MobileAdapterGB*) user)->p;

	uint32_t time = mTimingCurrentTime(&adapter->d.p->p->timing);
	uint32_t diff = time - adapter->timeLatch[timer];
	uint32_t cycles = (double) ms * GBA_ARM7TDMI_FREQUENCY / 1000;
	return diff >= cycles;
}

static bool GBASIOMobileAdapterInit(struct GBASIODriver* driver);
static void GBASIOMobileAdapterDeinit(struct GBASIODriver* driver);
static uint16_t GBASIOMobileAdapterWriteRegister(struct GBASIODriver* driver, uint32_t address, uint16_t value);

static void _mobileTransfer(struct GBASIOMobileAdapter* mobile, bool fastClock);
static void _mobileEvent(struct mTiming* timing, void* mobile, uint32_t cyclesLate);

void GBASIOMobileAdapterCreate(struct GBASIOMobileAdapter* mobile) {
	mobile->d.init = GBASIOMobileAdapterInit;
	mobile->d.deinit = GBASIOMobileAdapterDeinit;
	mobile->d.load = NULL;
	mobile->d.unload = NULL;
	mobile->d.writeRegister = GBASIOMobileAdapterWriteRegister;

	mobile->event.context = mobile;
	mobile->event.callback = _mobileEvent;
	mobile->event.priority = 0x80;

	memset(&mobile->m, 0, sizeof(mobile->m));
	mobile->m.p = mobile;
}

void GBASIOMobileAdapterUpdate(struct GBASIOMobileAdapter* mobile) {
	if (!mobile->m.adapter) return;
	mobile_loop(mobile->m.adapter);
}

bool GBASIOMobileAdapterInit(struct GBASIODriver* driver) {
	struct GBASIOMobileAdapter* mobile = (struct GBASIOMobileAdapter*) driver;
	mobile->m.adapter = MobileAdapterGBNew(&mobile->m);
	if (!mobile->m.adapter) return false;

	mobile_def_debug_log(mobile->m.adapter, debug_log);
	mobile_def_time_latch(mobile->m.adapter, time_latch);
	mobile_def_time_check_ms(mobile->m.adapter, time_check_ms);

	mobile_start(mobile->m.adapter);
	return true;
}

void GBASIOMobileAdapterDeinit(struct GBASIODriver* driver) {
	struct GBASIOMobileAdapter* mobile = (struct GBASIOMobileAdapter*) driver;
	if (!mobile->m.adapter) return;

	mobile_stop(mobile->m.adapter);
	free(mobile->m.adapter);
	mobile->m.adapter = NULL;
}

uint16_t GBASIOMobileAdapterWriteRegister(struct GBASIODriver* driver, uint32_t address, uint16_t value) {
	struct GBASIOMobileAdapter* mobile = (struct GBASIOMobileAdapter*) driver;
	if (address == GBA_REG_SIOCNT && (value & 0x81) == 0x81) {
		_mobileTransfer(mobile, value & 0x2);
	}
	return value;
}

void _mobileTransfer(struct GBASIOMobileAdapter* mobile, bool fastClock) {
	int32_t cycles = GBA_ARM7TDMI_FREQUENCY / 0x40000; // 2MHz
	if (!fastClock) cycles *= 8; // 256kHz
	if (mobile->d.p->mode == SIO_NORMAL_32) cycles *= 4; // Four bytes

	mTimingDeschedule(&mobile->d.p->p->timing, &mobile->event);
	mTimingSchedule(&mobile->d.p->p->timing, &mobile->event, cycles);
}

void _mobileEvent(struct mTiming* timing, void* user, uint32_t cyclesLate) {
	UNUSED(timing);
	struct GBASIOMobileAdapter* mobile = user;

	if (mobile->d.p->mode == SIO_NORMAL_32 && mobile->m.serial == 4) {
		uint16_t* reg_lo = &mobile->d.p->p->memory.io[GBA_REG_SIODATA32_LO >> 1];
		uint16_t* reg_hi = &mobile->d.p->p->memory.io[GBA_REG_SIODATA32_HI >> 1];
		uint32_t tmp = *reg_hi << 16 | *reg_lo;
		*reg_hi = mobile->next >> 16;
		*reg_lo = mobile->next;
		mobile->next = mobile_transfer_32bit(mobile->m.adapter, tmp);
	} else if (mobile->d.p->mode == SIO_NORMAL_8 && mobile->m.serial == 1) {
		uint16_t* reg = &mobile->d.p->p->memory.io[GBA_REG_SIODATA8 >> 1];
		uint8_t tmp = *reg;
		*reg = mobile->next;
		mobile->next = mobile_transfer(mobile->m.adapter, tmp);
	}

	mobile->d.p->siocnt = GBASIONormalClearStart(mobile->d.p->siocnt);
	if (GBASIONormalIsIrq(mobile->d.p->siocnt)) {
		GBARaiseIRQ(mobile->d.p->p, GBA_IRQ_SIO, cyclesLate);
	}
}
