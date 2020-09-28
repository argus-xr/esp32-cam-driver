#ifndef NETBUFFER_H
#define NETBUFFER_H

#include <cstdint>
#include <string>

class OutPacket {
public:
	OutPacket(uint8_t* inData, uint32_t inLength, void freeFunc(void* ptr));
	~OutPacket();
	uint32_t getDataLength();
	uint8_t* getData();
private:
	uint8_t* data;
	uint32_t length;
	void (*myFree)(void*);
};

namespace NetBufferDefaultMalloc {
	void* myMalloc(uint64_t size);
	void myFree(void* ptr);
};

class NetBuffer {
public:
	NetBuffer();
	NetBuffer(void* (*mallocFunction)(uint64_t), void (*freeFunction)(void*));
	virtual ~NetBuffer();
	void insertBuffer(uint8_t* buffer, uint32_t length, bool copyBuffer);
	void removeStartOfBuffer(uint32_t length);
	void setResizeStep(uint32_t step);
	uint32_t getLength();
	uint8_t getByteAt(uint32_t pos);
	uint8_t* extractBufferAt(uint32_t pos, uint32_t length);
	int32_t findByteSequence(const uint8_t* sequence, uint32_t sequenceLength, uint32_t startPos);
	static std::string debugBuffer(uint8_t* buffer, uint32_t length);
	static std::string debugBuffer(uint8_t* buffer, uint32_t length, uint32_t amount);

protected:
	uint8_t* internalBuffer = nullptr;
	uint32_t internalBufferContentSize = 0;
	uint32_t internalBufferMemorySize = 0;
	uint32_t bufferResizeStep = 128;

	void* (*myMalloc)(uint64_t);
	void (*myFree)(void*);
};

class NetMessageIn {
public:
	NetMessageIn(uint8_t* buffer, uint32_t length, void* (*mallocFunction)(uint64_t), void (*freeFunction)(void*));
	NetMessageIn(uint8_t* buffer, uint32_t length);
	~NetMessageIn();
	bool isValid();
	void setReadPos(uint32_t pos);
	template <typename T>
	T readuint();
	uint8_t readuint8();
	uint16_t readuint16();
	uint32_t readuint32();
	uint64_t readVarInt();
	uint8_t* readByteBlob(uint32_t length);
	std::string readVarString();
	uint8_t* getInternalBuffer();
	uint32_t getInternalBufferLength();
	std::string debugBuffer();
	std::string debugBuffer(uint32_t amount);
protected:
	uint8_t* internalBuffer;// = nullptr;
	uint32_t bufferLength = 0;
	uint32_t bufferPos = 0;

	void* (*myMalloc)(uint64_t);
	void (*myFree)(void*);
};

class NetMessageOut {
public:
	NetMessageOut(uint32_t length, void* (*mallocFunction)(uint64_t), void (*freeFunction)(void*));
	NetMessageOut(uint32_t length);
	~NetMessageOut();
	template <typename T>
	void writeuint(T val);
	void writeuint8(uint8_t val);
	void writeuint16(uint16_t val);
	void writeuint32(uint32_t val);
	void writeVarInt(uint64_t val);
	uint8_t bytesToFitVarInt(uint64_t val);
	void writeByteBlob(uint8_t* blob, uint32_t length);
	void writeVarString(std::string text);

	uint8_t* getInternalBuffer();
	uint32_t getContentLength();
	std::string debugBuffer();
	std::string debugBuffer(uint32_t amount);
protected:
	void reserveBufferSize(uint32_t requiredLength);
	void ensureSpaceFor(uint32_t extraBytes, bool exact = false);
	uint8_t* internalBuffer;// = nullptr;
	uint32_t bufferLength = 0;
	uint32_t bufferPos = 0;

	void* (*myMalloc)(uint64_t);
	void (*myFree)(void*);
};

#endif