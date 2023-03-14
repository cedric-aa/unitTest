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

void uartReceivethread(void)
{
    dataQueueItemType uartReceiveQueueItem;
    static uint8_t message[BUF_SIZE];
    static size_t messageLength = 0;
    static enum uart_state_t state = UART_STATE_IDLE;
    static uint8_t payloadSize = 0;
    // static uint8_t count = 0; // debug counter

    while (1)
    {

        k_msgq_get(&uartMsgq, &uartReceiveQueueItem, K_FOREVER);

        for (int i = 0; i < uartReceiveQueueItem.length; i++)
        {
            switch (state)
            {

            case UART_STATE_IDLE:
                if (uartReceiveQueueItem.bufferItem[i] == START_BYTE)
                {
                    state = UART_STATE_RECEIVING_LEN;
                }
                break;

            case UART_STATE_RECEIVING_LEN:
                payloadSize = uartReceiveQueueItem.bufferItem[i];
                if (payloadSize + END_OF_FRAME_SIZE > BUF_SIZE)
                {
                    LOG_ERR("Message too long");
                    state = UART_STATE_IDLE;
                    messageLength = 0;
                }
                else
                {
                    state = UART_STATE_RECEIVING;
                    message[messageLength++] = uartReceiveQueueItem.bufferItem[i];
                }
                break;

            case UART_STATE_RECEIVING:
                if (uartReceiveQueueItem.bufferItem[i] == END_BYTE &&
                    messageLength == payloadSize + END_OF_FRAME_SIZE)
                {
                    uint16_t *receivedCrc = (uint16_t *)&message[payloadSize + 1];
                    unsigned short crc = crc16Ccitt(&message[1], payloadSize);
                    if (crc == *receivedCrc)
                    {
                        // Process the message data here
                        LOG_INF("Received message");
                        //  LOG_HEXDUMP_INF(&message, messageLength, "success received");
                    }
                    else
                    {
                        LOG_ERR("Received message has invalid CRC16");
                    }

                    state = UART_STATE_IDLE;
                    messageLength = 0;
                }
                else if (uartReceiveQueueItem.bufferItem[i] != END_BYTE &&
                         messageLength >= payloadSize + END_OF_FRAME_SIZE)
                {
                    LOG_ERR("Error Framing ");
                    state = UART_STATE_IDLE;
                    messageLength = 0;
                }
                else if (messageLength < payloadSize + END_OF_FRAME_SIZE)
                {
                    message[messageLength++] = uartReceiveQueueItem.bufferItem[i];
                }
                break;

            default:

                break;
            }
        }
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

K_THREAD_DEFINE(uartReceivethread_id, THREAD_UART_STACKSIZE, uartReceivethread, NULL, NULL, NULL, THREAD_UART_PRIORITY, 0, 0);