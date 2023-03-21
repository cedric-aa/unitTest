#ifndef MESSAGE_FORMAT_AA_H__
#define MESSAGE_FORMAT_AA_H__

#include <stdint.h>
#include <string.h>

enum messageType { UNIT_CONTROL_TYPE = 1, ACTIVATION_TYPE = 2, SENSOR_TYPE = 3, MOTOR_LEVEL = 4 };

enum messageId {
	SET = 1,
	STATUS = 2,
	GET = 3,
	SETACK = 4,
};

typedef struct {
	bool isUartAck;
	uint16_t address;
	uint8_t messageType;
	uint8_t messageID;
	uint8_t payloadBuffer[16];
	size_t payloadLength;
	uint8_t sequenceNumber;
} processedMessage;

typedef struct {
	uint8_t bufferItem[16];
	int length;
} dataQueueItemType;

processedMessage processPublisherQueueItem(const dataQueueItemType *publisherQueueItem);
dataQueueItemType headerHubFormatUartTx(uint16_t addr, uint8_t messageType, uint8_t messageID,
					uint8_t uartAck);
dataQueueItemType headerCbFormatUartTx(uint8_t messageID);
#endif