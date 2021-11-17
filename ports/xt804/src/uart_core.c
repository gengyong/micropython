#include <unistd.h>
#include "py/mpconfig.h"

#include "wm_hal.h"
#include "hal_init.h"
#include "fifo.h"

/*
 * Core UART functions to implement for a port
 */

UART_HandleTypeDef huart;
#define IT_LEN (0) 	// 大于等于0，0：接收不定长数据即可触发中断回调；大于0：接收N个长度数据才触发中断回调
static uint8_t buf[32] = {0}; // 必须大于等于32字节
static uint8_t pdata[1024] = {0};

void HAL_UART0_Init() {
#if USE_UART0_PRINT
    huart.Instance = UART0;
#else
    huart.Instance = UART1;
#endif
	huart.Init.BaudRate = 115200;
	huart.Init.WordLength = UART_WORDLENGTH_8B;
	huart.Init.StopBits = UART_STOPBITS_1;
	huart.Init.Parity = UART_PARITY_NONE;
	huart.Init.Mode = UART_MODE_TX | UART_MODE_RX;
	huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	if (HAL_UART_Init(&huart) == HAL_OK)
	{
        FifoInit(pdata, 1024);
	    HAL_UART_Receive_IT(&huart, buf, IT_LEN);
	} else {
        Error_Handler();
    }
}


// Receive single character
int mp_hal_stdin_rx_chr(void) {
    unsigned char c = 0;
   
    // wait for RXNE
    if (FifoDataLen() > 0) {
        FifoRead(&c, 1);
    }
    
    return c;
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    while (len--) {
        // wait for TXE
        sendchar(*str++);
    }
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (FifoSpaceLen() >= huart->RxXferCount)
	{
		FifoWrite(huart->pRxBuffPtr, huart->RxXferCount);
	}
}
