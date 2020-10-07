#ifndef NETUTILS_H
#define NETUTILS_H

#include "stdint.h"

namespace ArgusNetUtils {
	template <typename T>
	T toNetworkOrder(T val);
	template <typename T>
	T toHostOrder(T val);
	uint8_t writeVarInt(uint8_t* buf, uint64_t val);
	uint8_t readVarInt(uint8_t* buf, uint64_t &val);
	uint8_t writeVarIntSigned(uint8_t* buf, int64_t val);
	uint8_t readVarIntSigned(uint8_t* buf, int64_t &val);
	uint8_t bytesToFitVarInt(uint64_t val);
	uint8_t bytesToFitVarIntSigned(int64_t val);
}

#endif // NETUTILS_H