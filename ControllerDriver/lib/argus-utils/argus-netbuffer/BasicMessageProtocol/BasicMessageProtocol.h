#ifndef BASICMESSAGEPROTOCOL_H
#define BASICMESSAGEPROTOCOL_H

#include "../netbuffer.h"

class BasicMessageBuffer : public NetBuffer {
public:
	BasicMessageBuffer();
	BasicMessageBuffer(void* (*mallocFunction)(uint64_t), void (*freeFunction)(void*));
	~BasicMessageBuffer();
	void checkMessages();
	NetMessageIn* popMessage();
	OutPacket* messageToOutPacket(NetMessageOut* msg);
	
	const uint8_t startSequence[2] = { '\\', 0 };
	const uint32_t startSequenceLength = 2;
	const uint8_t endSequence[2] = { '\\', 1 };
	const uint32_t endSequenceLength = 2;
	const uint8_t escapeCharacter = '\\';
protected:
	NetMessageIn** messageList = nullptr;
	uint8_t messageListNum = 0;
	uint8_t messageListMax = 5;
	uint8_t messageListPos = 0;
	void resizeMessageList(uint8_t size);
};

#endif // BASICMESSAGEPROTOCOL_H