#ifndef GBA_MOBILE_H
#define GBA_MOBILE_H

#include <mgba-util/common.h>

CXX_GUARD_START

#include <mgba/gba/interface.h>
#include <mgba-util/mobile.h>

struct GBASIOMobileAdapter {
	struct GBASIODriver d;
	struct mTimingEvent event;
	struct MobileAdapterGB m;
	uint32_t timeLatch[MOBILE_MAX_TIMERS];
	uint32_t next;
};

void GBASIOMobileAdapterCreate(struct GBASIOMobileAdapter*);

CXX_GUARD_END

#endif
