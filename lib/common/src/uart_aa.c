#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include "uart_aa.h"
#include "check_crc_aa.h"

LOG_MODULE_REGISTER(uart_AA, LOG_LEVEL_DBG);

#define THREAD_UART_STACKSIZE 1024
#define THREAD_UART_PRIORITY 14
#define RECEIVE_TIMEOUT 1000
#define BUF_SIZE 16
#define START_BYTE 0x7E
#define END_BYTE 0x7F
#define END_OF_FRAME_SIZE 3 // include crc1 + crc2 +  END_BYTE

const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart0));

static uint8_t doubleBuffer[2][BUF_SIZE];
static uint8_t *nextBuf = doubleBuffer[1];

typedef struct
{
    uint8_t bufferItem[BUF_SIZE];
    int length;
} dataQueueItemType;

K_MSGQ_DEFINE(uartMsgq, sizeof(dataQueueItemType), 10, 4);

enum uart_state_t
{
    UART_STATE_IDLE,
    UART_STATE_RECEIVING,
    UART_STATE_RECEIVING_LEN
};

static void uartStateMachine(uint8_t *buff, uint8_t buffLength)
{
    static uint8_t message[BUF_SIZE];
    static size_t messageLength = 0;
    static enum uart_state_t state = UART_STATE_IDLE;
    static uint8_t payloadSize = 0;

    // LOG_HEXDUMP_INF(buff, buffLength, "start in buffer");
    //  LOG_HEXDUMP_INF(&message, messageLength, " start in message");

    for (int i = 0; i < buffLength; i++)
    {
        switch (state)
        {

        case UART_STATE_IDLE:
            if (buff[i] == START_BYTE)
            {
                state = UART_STATE_RECEIVING_LEN;
                message[messageLength++] = buff[i];
            }
            break;

        case UART_STATE_RECEIVING_LEN:
            payloadSize = buff[i];

            if (payloadSize + END_OF_FRAME_SIZE + 2 > BUF_SIZE)
            {
                LOG_ERR("payload size wrong can't be more than the BUF_SIZE %02x", payloadSize);
                state = UART_STATE_IDLE;
                messageLength = 0;
            }
            else
            {
                state = UART_STATE_RECEIVING;
                message[messageLength++] = buff[i];
            }
            break;

        case UART_STATE_RECEIVING:
            message[messageLength++] = buff[i];

            if (buff[i] == END_BYTE &&
                messageLength == payloadSize + END_OF_FRAME_SIZE + 2)
            {

                state = UART_STATE_IDLE;
                messageLength = 0;
                uint16_t *receivedCrc = (uint16_t *)&message[payloadSize + 2];
                unsigned short crc = crc16Ccitt(&message[2], payloadSize);
                if (crc == *receivedCrc)
                {
                    LOG_INF("Received message");
                }
                else
                {
                    LOG_ERR("Received message has invalid CRC16 [%04x] [%04x] ", crc, *receivedCrc);
                   }
            }
            else if (buff[i] != END_BYTE &&
                     messageLength >= payloadSize + END_OF_FRAME_SIZE + 2)
            {
                LOG_ERR("Error Framing ");
                state = UART_STATE_IDLE;
                int temp = messageLength;
                messageLength = 0;

                // look for another START_BYTE in the message buffer
                for (int j = 1; j < temp - 1; j++)
                {
                    if (message[j] == START_BYTE)
                    {
                        uartStateMachine(&message[j], temp - j);
                        break;
                    }
                }
            }

            break;

        default:

            break;
        }
    }
}

void uartReceiveThread(void)
{
    dataQueueItemType uartReceiveQueueItem;

    while (1)
    {

        k_msgq_get(&uartMsgq, &uartReceiveQueueItem, K_FOREVER);
        uartStateMachine(uartReceiveQueueItem.bufferItem, uartReceiveQueueItem.length);
    }
}

int uartInit(void)
{
    int ret;

    if (!device_is_ready(uart))
    {
        LOG_ERR("UART device not ready");
        return 1;
    }
    ret = uart_callback_set(uart, uartHandler, NULL);
    if (ret)
    {
        return 1;
    }
    ret = uart_rx_enable(uart, doubleBuffer[1], BUF_SIZE, RECEIVE_TIMEOUT);
    if (ret)
    {
        return 1;
    }
    return 0;
}

void uartHandler(const struct device *dev, struct uart_event *evt, void *user_data)
{
    int err;
    dataQueueItemType uartReceiveQueueItem;

    switch (evt->type)
    {
    case UART_TX_DONE:

        break;
    case UART_RX_RDY:
        uartReceiveQueueItem.length = evt->data.rx.len;
        memcpy(uartReceiveQueueItem.bufferItem, evt->data.rx.buf + evt->data.rx.offset, uartReceiveQueueItem.length);
        err = k_msgq_put(&uartMsgq, &uartReceiveQueueItem, K_NO_WAIT);
        if (err)
        {
            LOG_ERR("UART message queue full, dropping data.");
        }

        break;
    case UART_RX_BUF_REQUEST:
        nextBuf = (nextBuf == doubleBuffer[0]) ? doubleBuffer[1] : doubleBuffer[0];
        err = uart_rx_buf_rsp(uart, nextBuf, BUF_SIZE);
        __ASSERT(err == 0, "Failed to provide new buffer");
        break;
    case UART_RX_BUF_RELEASED:
        break;
    case UART_RX_DISABLED:
        break;
    default:
        break;
    }
}

K_THREAD_DEFINE(uartReceiveThread_id, THREAD_UART_STACKSIZE, uartReceiveThread, NULL, NULL, NULL, THREAD_UART_PRIORITY, 0, 0);