#ifndef MESSAGE_FORMAT_AA_H__
#define MESSAGE_FORMAT_AA_H__

#include <stdint.h>
#include <string.h>

//ErrorCode
#define WAITING_FOR_RESPONSE 0x04
#define SET_OPERATION_SUCCESS 0x00
#define SET_TIMEOUT 0x01
#define CB_CPU_NOT_RESPONDING 0x03



enum messageType { UNIT_CONTROL_TYPE = 1, ACTIVATION_TYPE = 2, SENSOR_TYPE = 3, MOTOR_TYPE = 4 };


enum messageId {
	SET = 1,
	STATUS = 2,
	GET = 3,
	STATUS_CODE = 4,
	STATUS_ID = 5,
	GET_ID = 6,
};






typedef struct {
	bool isUartAck;
	uint16_t address;
	uint8_t messageType;
	uint8_t messageID;
	uint8_t payloadBuffer[32];
	size_t payloadLength;
	uint8_t sequenceNumber;
} processedMessage;

typedef struct {
	uint8_t bufferItem[32];
	int length;
} dataQueueItemType;

dataQueueItemType createPublisherQueueItem(bool uartAck, uint16_t addr, uint8_t messageType,
					   uint8_t messageId, uint8_t *buf, uint8_t len);
processedMessage processPublisherQueueItem(const dataQueueItemType *publisherQueueItem);
dataQueueItemType headerFormatUartTx(uint16_t addr, uint8_t messageType, uint8_t messageID,
				     uint8_t uartAck);
int forwardToUart(uint8_t uartAck, uint16_t addr, uint8_t messageType, uint8_t messageId,
		  uint8_t *buf, uint8_t len);
#endif