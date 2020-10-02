#include "BasicMessageProtocol.h"
#include <cstring>

#include "argus-netbuffer/netutils.h"
#include <limits.h>


BasicMessageBuffer::BasicMessageBuffer()
		: NetBuffer() {
}

BasicMessageBuffer::BasicMessageBuffer(void* (*mallocFunction)(uint64_t), void (*freeFunction)(void*))
		: NetBuffer(mallocFunction, freeFunction) {
}

BasicMessageBuffer::~BasicMessageBuffer() {
}

void BasicMessageBuffer::checkMessages() {
	int32_t pos = findByteSequence(startSequence, startSequenceLength, 0);
	int32_t chop = -1;
	if (pos >= 0) {
		bool posFound = false;
		int32_t endpos = 1;
		while (!posFound && endpos > 0) { // endpos < 0 means there's no more matching sequences.
			endpos = findByteSequence(endSequence, endSequenceLength, endpos + 1);
			if (endpos > 0 && getByteAt(endpos - 1) != '\\') { // check if the \ character is escaped, which would make the \0 not count.
				posFound = true;
			}
		}
		if (posFound) {
			chop = endpos + endSequenceLength;
			uint64_t length = 0; // length VarInt, used to verify that the entire message was received. Not used yet.
			uint8_t varIntBytes = ArgusNetUtils::readVarInt(internalBuffer + startSequenceLength, length); // read a VarInt into length. The number of bytes used is stored in bytes.
			int32_t messageLength = endpos - (startSequenceLength + varIntBytes) - pos; // endpos is the start of the end-sequence, so don't subtract endSequenceLength.
			if (messageLength >= length) { // message with escaped characters is longer than or equal to expected length
				uint8_t* nmbuf = new uint8_t[messageLength];
				uint32_t offset = 0;
				uint32_t bufOffset = varIntBytes + startSequenceLength;
				for (int i = 0; i < messageLength; ++i) {
					if (internalBuffer[bufOffset + i] == escapeCharacter) {
						if (i < messageLength - 1 && internalBuffer[bufOffset + i + pos + 1] == escapeCharacter) {
							nmbuf[i - offset] = escapeCharacter;
							offset += 1;
							i++; // skip one step because we already covered it.
						}
					}
					nmbuf[i - offset] = internalBuffer[bufOffset + i];
				}
				messageLength -= offset; // actual message length without escapes.
				if (messageLength == length) {
					NetMessageIn* newMessage = new NetMessageIn(nmbuf, messageLength, myMalloc, myFree);
					messageList.push(newMessage);
				}
			}
		}
	}
	else if (internalBufferContentSize > 100000) { // there's 100000 bytes worth of garbage in here that won't ever match the protocol, ditch it!
		chop = internalBufferContentSize - startSequenceLength; // there could be a partial valid delimiter at the end, so don't delete everything.
	}
	if (chop > 0) {
		removeStartOfBuffer(chop);
	}
}

NetMessageIn* BasicMessageBuffer::popMessage() {
	if (messageList.empty()) {
		return nullptr;
	}
	NetMessageIn* ret = messageList.top();
	messageList.pop();
	return ret;
}

OutPacket* BasicMessageBuffer::messageToOutPacket(NetMessageOut* msg) {
	uint32_t pSize = msg->getContentLength();
	uint8_t varIntSize = msg->bytesToFitVarInt(pSize);
	uint8_t* varIntBuf = new uint8_t[varIntSize];
	ArgusNetUtils::writeVarInt(varIntBuf, pSize);

	uint32_t extra = 0;

	for(int i = 0; i < varIntSize; ++i) {
		if(varIntBuf[i] == escapeCharacter) {
			extra ++; // We need an extra spot for an escape character.
		}
	}

	uint8_t* msgbuf = msg->getInternalBuffer();
	for(int i = 0; i < pSize; ++i) {
		if(msgbuf[i] == escapeCharacter) {
		extra ++; // We need an extra spot for an escape character.
		}
	}
	
	uint64_t outBufSize = (uint64_t) startSequenceLength + varIntSize + pSize + endSequenceLength + extra;
	if (outBufSize > ULONG_MAX) {
		return 0; // message size has to fit in uint32_t. Throw something?
	}
	uint8_t* outBuf = (uint8_t*) myMalloc(outBufSize);

	extra = 0; // now using this to track the offset from escape characters.
	for(int i = 0; i < startSequenceLength; ++i) {
		outBuf[i] = startSequence[i];
	}
	uint32_t start = startSequenceLength;

	for(int i = 0; i < varIntSize; ++i) {
		uint8_t byte = varIntBuf[i];
		if(byte == escapeCharacter) {
			outBuf[start + i + extra] = escapeCharacter; // insert an escape before the actual escape character, so it represents the actual character. \\ becomes \.
			extra ++;
		}
		outBuf[start + i + extra] = byte;
	}
	start += varIntSize;

	for(int i = 0; i < pSize; ++i) {
		uint8_t byte = msgbuf[i];
		if(byte == escapeCharacter) {
			outBuf[start + i + extra] = escapeCharacter; // see above.
			extra ++;
		}
		outBuf[start + i + extra] = byte;
	}
	start += pSize;
	
	for(int i = 0; i < endSequenceLength; ++i) {
		outBuf[start + i + extra] = endSequence[i];
	}

	OutPacket* p = new OutPacket(outBuf, outBufSize, myFree);

	return p;
}