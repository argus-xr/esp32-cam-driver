#ifndef BASICMESSAGEPROTOCOL_H
#define BASICMESSAGEPROTOCOL_H

#include "../netbuffer.h"
#include <stack>

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
	std::stack<NetMessageIn*> messageList;
};

#endif // BASICMESSAGEPROTOCOL_H