#include <mgba/internal/gba/sio/mobile.h>

#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/io.h>

mLOG_DECLARE_CATEGORY(GBA_MOBILE);
mLOG_DEFINE_CATEGORY(GBA_MOBILE, "Mobile Adapter (GBA)", "gba.mobile");

static void debug_log(void* user, const char* line) {
	UNUSED(user);
	mLOG(GBA_MOBILE, DEBUG, "%s", line);
}

static void serial_disable(void* user) {
	USER1.serial = 0;
}

static void serial_enable(void* user, bool mode_32bit) {
	USER1.serial = mode_32bit ? 4 : 1;
	USER1.nextData = MOBILE_SERIAL_IDLE_WORD;
}

static bool config_read(void* user, void* dest, uintptr_t offset, size_t size) {
	return memcpy(dest, USER1.config + offset, size) == dest;
}

static bool config_write(void* user, const void* src, uintptr_t offset, size_t size) {
	return memcpy(USER1.config + offset, src, size) == USER1.config + offset;
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
static bool GBASIOMobileAdapterHandlesMode(struct GBASIODriver* driver, enum GBASIOMode mode);
static int GBASIOMobileAdapterConnectedDevices(struct GBASIODriver* driver);
static uint16_t GBASIOMobileAdapterWriteSIOCNT(struct GBASIODriver* driver, uint16_t value);
static uint8_t GBASIOMobileAdapterFinishNormal8(struct GBASIODriver* driver);
static uint32_t GBASIOMobileAdapterFinishNormal32(struct GBASIODriver* driver);

void GBASIOMobileAdapterCreate(struct GBASIOMobileAdapter* mobile) {
	mobile->d.init = GBASIOMobileAdapterInit;
	mobile->d.deinit = GBASIOMobileAdapterDeinit;
	mobile->d.load = NULL;
	mobile->d.unload = NULL;
	mobile->d.handlesMode = GBASIOMobileAdapterHandlesMode;
	mobile->d.connectedDevices = GBASIOMobileAdapterConnectedDevices;
	mobile->d.writeSIOCNT = GBASIOMobileAdapterWriteSIOCNT;
	mobile->d.finishNormal8 = GBASIOMobileAdapterFinishNormal8;
	mobile->d.finishNormal32 = GBASIOMobileAdapterFinishNormal32;

	memset(&mobile->m, 0, sizeof(mobile->m));
	mobile->m.p = mobile;
}

void GBASIOMobileAdapterUpdate(struct GBASIOMobileAdapter* mobile) {
	if (!mobile->m.adapter) return;
	mobile_loop(mobile->m.adapter);
}

static bool GBASIOMobileAdapterInit(struct GBASIODriver* driver) {
	struct GBASIOMobileAdapter* mobile = (struct GBASIOMobileAdapter*) driver;
	mobile->m.adapter = MobileAdapterGBNew(&mobile->m);
	if (!mobile->m.adapter) return false;

	mobile_def_debug_log(mobile->m.adapter, debug_log);
	mobile_def_time_latch(mobile->m.adapter, time_latch);
	mobile_def_time_check_ms(mobile->m.adapter, time_check_ms);

	mobile_start(mobile->m.adapter);

	mobile->d.p->magb = mobile;
	return true;
}

static void GBASIOMobileAdapterDeinit(struct GBASIODriver* driver) {
	struct GBASIOMobileAdapter* mobile = (struct GBASIOMobileAdapter*) driver;
	if (!mobile->m.adapter) return;
	mobile->d.p->magb = NULL;

	mobile_stop(mobile->m.adapter);
	free(mobile->m.adapter);
	mobile->m.adapter = NULL;
}

static bool GBASIOMobileAdapterHandlesMode(struct GBASIODriver* driver, enum GBASIOMode mode) {
	UNUSED(driver);
	switch (mode) {
	case GBA_SIO_NORMAL_8:
	case GBA_SIO_NORMAL_32:
		return true;
	default:
		return false;
	}
}

static int GBASIOMobileAdapterConnectedDevices(struct GBASIODriver* driver) {
	UNUSED(driver);
	return 1;
}

static uint16_t GBASIOMobileAdapterWriteSIOCNT(struct GBASIODriver* driver, uint16_t value) {
	UNUSED(driver);
	return value | 4;
}

static uint8_t GBASIOMobileAdapterFinishNormal8(struct GBASIODriver* driver) {
	struct GBASIOMobileAdapter* mobile = (struct GBASIOMobileAdapter*) driver;

	if (mobile->m.serial == 1) {
		uint8_t reg = mobile->d.p->p->memory.io[GBA_REG(SIODATA8)];
		uint8_t ret = mobile->next;
		mobile->next = mobile_transfer(mobile->m.adapter, reg);
		return ret;
	}

	return 0xFF;
}

static uint32_t GBASIOMobileAdapterFinishNormal32(struct GBASIODriver* driver) {
	struct GBASIOMobileAdapter* mobile = (struct GBASIOMobileAdapter*) driver;

	if (mobile->m.serial == 4) {
		uint16_t reg_lo = mobile->d.p->p->memory.io[GBA_REG(SIODATA32_LO)];
		uint16_t reg_hi = mobile->d.p->p->memory.io[GBA_REG(SIODATA32_HI)];
		uint32_t ret = mobile->next;
		mobile->next = mobile_transfer_32bit(mobile->m.adapter, reg_hi << 16 | reg_lo);
		return ret;
	}

	return 0xFFFFFFFF;
}
