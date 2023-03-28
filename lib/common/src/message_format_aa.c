#include "message_format_aa.h"

processedMessage processPublisherQueueItem(const dataQueueItemType *publisherQueueItem)
{
	processedMessage processedMessage;
	// Extract the UART acknowledgement flag from the first byte of the bufferItem array
	processedMessage.isUartAck = publisherQueueItem->bufferItem[0];

	// Extract the address from the second and third bytes of the bufferItem array and store it as a uint16_t value
	processedMessage.address = *((uint16_t *)&publisherQueueItem->bufferItem[1]);

	// Extract the message type from the fourth byte of the bufferItem array
	processedMessage.messageType = publisherQueueItem->bufferItem[3];

	// Extract the message ID from the fifth byte of the bufferItem array
	processedMessage.messageID = publisherQueueItem->bufferItem[4];

	// Calculate the length of the payload and create a buffer for it
	processedMessage.payloadLength = publisherQueueItem->length - 5;

	// Copy the payload to the payloadBuffer
	memcpy(processedMessage.payloadBuffer, &publisherQueueItem->bufferItem[5],
	       processedMessage.payloadLength);

	return processedMessage;
}

dataQueueItemType headerFormatUartTx(uint16_t addr, uint8_t messageType, uint8_t messageID,
					uint8_t uartAck)
{
	dataQueueItemType uartTxQueueItem;
	uartTxQueueItem.length = 6;
	uartTxQueueItem.bufferItem[0] = 5; // payload length
	uartTxQueueItem.bufferItem[1] = 0; // uartAck
	uartTxQueueItem.bufferItem[2] = (uint8_t)(addr & 0xFF); // low byte of address
	uartTxQueueItem.bufferItem[3] = (uint8_t)((addr >> 8) & 0xFF); // high byte of address
	uartTxQueueItem.bufferItem[4] = messageType; // message type
	uartTxQueueItem.bufferItem[5] = messageID; // message ID

	return uartTxQueueItem;
}

