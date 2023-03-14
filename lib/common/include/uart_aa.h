#ifndef UART_AA_H__
#define UART_AA_H__

#include <zephyr/drivers/uart.h>

int uartInit(void);
void uartHandler(const struct device *dev, struct uart_event *evt, void *user_data);

#endif
