#include "netbuffer.h"
#include <cstring>

#include "netutils.h"
#include "stdexcept"

void NetBuffer::insertBuffer(uint8_t* buffer, uint32_t length, bool copyBuffer) {
	uint32_t neededSize = length + internalBufferContentSize;
	if (internalBuffer == nullptr || neededSize > internalBufferMemorySize) {
		uint8_t* oldBuffer = internalBuffer;
		uint32_t newSize = neededSize + bufferResizeStep;
		internalBuffer = new uint8_t[newSize];
		if (oldBuffer != nullptr) {
			std::memcpy(internalBuffer, oldBuffer, internalBufferContentSize);
		}
		internalBufferMemorySize = neededSize + bufferResizeStep;
	}
	std::memcpy(internalBuffer + internalBufferContentSize, buffer, length);
	internalBufferContentSize += length;

	if (!copyBuffer) {
		// This is not actually supported yet, but if false, the inserted buffer is handed over.
		// That means the caller is no longer responsible for it, so we're deleting it here.
		// This will probably be supported in later, more efficient implementations.
		delete[] buffer;
	}
}

void NetBuffer::removeStartOfBuffer(uint32_t length) {
	if (internalBuffer == nullptr) {
		return;
	}
	else if (length >= internalBufferContentSize) {
		delete[] internalBuffer;
		internalBuffer = nullptr;
		internalBufferContentSize = 0;
		internalBufferMemorySize = 0;
	}
	else {
		uint8_t* oldBuffer = internalBuffer;

		internalBufferContentSize = internalBufferContentSize - length; // don't need the old values anymore.
		internalBufferMemorySize = internalBufferContentSize + bufferResizeStep;

		internalBuffer = new uint8_t[internalBufferMemorySize];
		std::memcpy(internalBuffer, oldBuffer + length, internalBufferContentSize);
	}
}

uint32_t NetBuffer::getLength() {
	return internalBufferContentSize;
}

uint8_t NetBuffer::getByteAt(uint32_t pos) {
	if (internalBuffer != nullptr && internalBufferContentSize > pos) {
		return internalBuffer[pos];
	}
	return 0; // Internal exception handling code is not enabled by default on Arduino platform, and uses a lot of memory. Need alternative?
}

uint8_t* NetBuffer::extractBufferAt(uint32_t pos, uint32_t length) {
	if (internalBuffer != nullptr && internalBufferContentSize > pos) {
		uint8_t* result = new uint8_t[length];
		uint32_t copiedBytes = length < internalBufferContentSize ? length : internalBufferContentSize;
		memcpy(result, internalBuffer + pos, copiedBytes);
		if (copiedBytes < length) {
			memset(result, 0, length - copiedBytes);
		}
	}
	return nullptr;
}

int32_t NetBuffer::findByteSequence(const uint8_t* sequence, uint32_t sequenceLength, uint32_t startPos) {
	if (internalBuffer != nullptr) {
		for (uint32_t i = startPos; i <= internalBufferContentSize - sequenceLength; ++i) {
			for (uint32_t j = 0; j < sequenceLength; ++j) {
				if (internalBuffer[i + j] != sequence[j]) {
					break; // break J loop, the sequence isn't here.
				}
				else if (j == sequenceLength - 1) { // checked the last byte of the sequence, still matches.
					return i;
				}
			}
		}
	}
	return -1;
}

void NetBuffer::setResizeStep(uint32_t step) {
	bufferResizeStep = step;
}



// NETMESSAGEIN

NetMessageIn::NetMessageIn(uint8_t* buffer, uint32_t length) {
	internalBuffer = buffer;
	bufferLength = length;
}

NetMessageIn::~NetMessageIn() {
	if(internalBuffer != nullptr) {
		delete[] internalBuffer;
		internalBuffer = nullptr;
	}
}

bool NetMessageIn::isValid() {
	return (internalBuffer != nullptr);
}

void NetMessageIn::setReadPos(uint32_t pos) {
	bufferPos = pos;
}

template <>
uint8_t NetMessageIn::readuint<uint8_t>() {
	if (bufferLength < bufferPos + 1) {
		// fail
		return 0;
	}
	bufferPos += 1;
	return internalBuffer[bufferPos - 1];
}

template <typename T>
T NetMessageIn::readuint() {
	if (bufferLength < bufferPos + sizeof(T)) {
		throw new std::length_error(std::string("Reading beyond end of buffer."));
	}
	T tmp = 0;
	for (int i = 0; i < sizeof(T); ++i) {
		tmp |= internalBuffer[bufferPos + i] << (sizeof(T) - i - 1) * 8;
	}
	bufferPos += sizeof(T);
	return ArgusNetUtils::toHostOrder<T>(tmp);
}

uint8_t NetMessageIn::readuint8() {
	return readuint<uint8_t>();
}

uint16_t NetMessageIn::readuint16() {
	if (bufferLength < bufferPos + 2) {
		// fail
		return 0;
	}
	uint16_t tmp = 0;
	tmp |= internalBuffer[bufferPos] << 8;
	tmp |= internalBuffer[bufferPos + 1];
	bufferPos += 2;
	return ArgusNetUtils::toHostOrder(tmp);
}

uint32_t NetMessageIn::readuint32() {
	if (bufferLength < bufferPos + 4) {
		// fail
		return 0;
	}
	uint32_t tmp = 0;
	tmp |= internalBuffer[bufferPos] << 24;
	tmp |= internalBuffer[bufferPos + 1] << 16;
	tmp |= internalBuffer[bufferPos + 2] << 8;
	tmp |= internalBuffer[bufferPos + 3];
	bufferPos += 4;
	return ArgusNetUtils::toHostOrder(tmp);
}

uint64_t NetMessageIn::readVarInt() {
	uint64_t val;
	bufferPos += ArgusNetUtils::readVarInt(internalBuffer + bufferPos, val); //TODO: Tell it how many bytes it can read, so it can stop and error if it would go out of bounds.
	return val;
}

uint8_t* NetMessageIn::readByteBlob(uint32_t length) {
	if (bufferLength < bufferPos + length) {
		return nullptr;
		// fail?
	}
	uint8_t* blob = new uint8_t[length];
	std::memcpy(blob, internalBuffer + bufferPos, length);
	return blob;
}

std::string NetMessageIn::readVarString() {
	uint64_t length = readVarInt();
	if (bufferLength < bufferPos + length) {
		return "";
		// fail?
	}
	uint8_t* blob = new uint8_t[length + 1]; // for /0 character
	blob[length] = '\0';
	std::memcpy(blob, internalBuffer + bufferPos, length);
	return std::string((char*)blob, length + 1);
}

uint8_t* NetMessageIn::getInternalBuffer() {
	return internalBuffer;
}

uint32_t NetMessageIn::getInternalBufferLength() {
	return bufferLength;
}



// NETMESSAGEOUT

NetMessageOut::NetMessageOut(uint32_t length) {
	if (length > 0) {
		internalBuffer = new uint8_t[length];
		bufferLength = length;
	}
	else {
		internalBuffer = new uint8_t[128]; // no dealing with null pointers.
		bufferLength = 128;
	}
}

NetMessageOut::~NetMessageOut() {
	if(internalBuffer != nullptr) {
		delete[] internalBuffer;
		internalBuffer = nullptr;
	}
}

template <>
void NetMessageOut::writeuint<uint8_t>(uint8_t val) {
	ensureSpaceFor(1);
	internalBuffer[bufferPos] = val;
	bufferPos += 1;
}

template <typename T>
void NetMessageOut::writeuint(T val) {
	ensureSpaceFor(sizeof(T));
	val = ArgusNetUtils::toNetworkOrder<T>(val);
	for (int i = 0; i < sizeof(T); ++i) {
		internalBuffer[bufferPos + i] = (uint8_t)(val >> (sizeof(T) - i - 1) * 8);
	}
	bufferPos += sizeof(T);
}

void NetMessageOut::writeuint8(uint8_t val) {
	ensureSpaceFor(1);
	internalBuffer[bufferPos] = val;
	bufferPos += 1;
}
void NetMessageOut::writeuint16(uint16_t val) {
	uint16_t netOrder = ArgusNetUtils::toNetworkOrder(val);
	internalBuffer[bufferPos] = (uint8_t) (netOrder >> 8);
	internalBuffer[bufferPos + 1] = (uint8_t) (netOrder);
	bufferPos += 2;
}
void NetMessageOut::writeuint32(uint32_t val) {
	uint32_t netOrder = ArgusNetUtils::toNetworkOrder(val);
	internalBuffer[bufferPos] = (uint8_t) (netOrder >> 24);
	internalBuffer[bufferPos + 1] = (uint8_t) (netOrder >> 16);
	internalBuffer[bufferPos + 2] = (uint8_t) (netOrder >> 8);
	internalBuffer[bufferPos + 3] = (uint8_t) (netOrder);
	bufferPos += 4;
}
void NetMessageOut::writeVarInt(uint64_t val) {
	ensureSpaceFor(bytesToFitVarInt(val));
	bufferPos += ArgusNetUtils::writeVarInt(internalBuffer + bufferPos, val);
}
uint8_t NetMessageOut::bytesToFitVarInt(uint64_t val) {
	return ArgusNetUtils::bytesToFitVarInt(val);
}
void NetMessageOut::writeByteBlob(uint8_t* blob, uint32_t length) {
	ensureSpaceFor(length);
	std::memcpy(internalBuffer + bufferPos, blob, length);
	bufferPos += length;
}
void NetMessageOut::writeVarString(std::string text) {
	uint64_t length = text.length(); // returns length without trailing /0, which is fine.
	writeVarInt(length);
	writeByteBlob((uint8_t*) text.c_str(), length);
}
uint8_t* NetMessageOut::getInternalBuffer() {
	return internalBuffer;
}
uint32_t NetMessageOut::getContentLength() {
	return bufferPos;
}
void NetMessageOut::reserveBufferSize(uint32_t requiredLength) {
	if (requiredLength > bufferLength) {
		uint8_t* tmp = internalBuffer;
		internalBuffer = new uint8_t[requiredLength];
		std::memcpy(internalBuffer, tmp, bufferPos);
		bufferLength = requiredLength;
		delete[] tmp;
	}
}

void NetMessageOut::ensureSpaceFor(uint32_t extraBytes, bool exact) {
	if (bufferPos + extraBytes > bufferLength) {
		if (exact) {
			reserveBufferSize(bufferPos + extraBytes);
		}
		else {
			reserveBufferSize(bufferPos + extraBytes + 128); // reserve some extra to reduce reallocations.
		}
	}
}