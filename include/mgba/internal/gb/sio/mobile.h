#ifndef GB_MOBILE_H
#define GB_MOBILE_H

#include <mgba-util/common.h>

CXX_GUARD_START

#include <mgba/gb/interface.h>
#include <mgba-util/mobile.h>

struct GBSIOMobileAdapter {
	struct GBSIODriver d;
	struct MobileAdapterGB m;
	uint32_t timeLatch[MOBILE_MAX_TIMERS];
	uint8_t byte;
	uint8_t next;
};

void GBSIOMobileAdapterCreate(struct GBSIOMobileAdapter*);
void GBSIOMobileAdapterUpdate(struct GBSIOMobileAdapter*);

CXX_GUARD_END

#endif
