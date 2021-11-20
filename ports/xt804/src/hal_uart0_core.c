#include <unistd.h>
#include "py/mpconfig.h"

#include "wm_hal.h"
#include "hal_common.h"
#include "hal_uart0_core.h"
#include "hal_uart0_fifo.h"
#include "libc_port.h"
#include "utf8_map.h"

// ================ configuration section ===
// define REPL(listen on uart0) input buffer size
#define REPL_INPUT_BUFFER_SIZE  (512)
// ==========================================
static int repl_buffer_tail = 0;
static int repl_buffer_iter = 0;
static uint8_t repl_buffer[REPL_INPUT_BUFFER_SIZE];
//===========================================
// IRQ: UART0
//===========================================
UART_HandleTypeDef huart;
__attribute__((isr)) void UART0_IRQHandler(void) {
	HAL_UART_IRQHandler(&huart);
}

//===========================================
static uint8_t uart0_buffer[32] = {0}; // 必须大于等于32字节
void HAL_UART0_Init() {
    huart.Instance = UART0;
	huart.Init.BaudRate = 115200;
	huart.Init.WordLength = UART_WORDLENGTH_8B;
	huart.Init.StopBits = UART_STOPBITS_1;
	huart.Init.Parity = UART_PARITY_NONE;
	huart.Init.Mode = UART_MODE_TX | UART_MODE_RX;
	huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	if (HAL_UART_Init(&huart) == HAL_OK) {
		repl_buffer_tail = 0;
		repl_buffer_iter = 0;
	    HAL_UART_Receive_IT(&huart, uart0_buffer, 0);
	} else {
        hal_panic("UART0: init failed.");
    }
}

void HAL_UART0_DeInit() {
	HAL_UART_DeInit(&huart);
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	// if (FifoSpaceLen() >= huart->RxXferCount) {
	// 	FifoWrite(huart->pRxBuffPtr, huart->RxXferCount);
	// }
	if (repl_buffer_tail + huart->RxXferCount <= sizeof(repl_buffer)) {
		memcpy(repl_buffer + repl_buffer_tail, huart->pRxBuffPtr, huart->RxXferCount);
		repl_buffer_tail += huart->RxXferCount;
	} else {
		if (repl_buffer_iter > 0) {
			int left = repl_buffer_tail - repl_buffer_iter;
			memcpy(repl_buffer, repl_buffer + repl_buffer_iter, left);
			repl_buffer_iter = 0;
			repl_buffer_tail = left;
		}
		int left = sizeof(repl_buffer) - repl_buffer_tail;
		if (left >= huart->RxXferCount) {
			left = huart->RxXferCount;
		} else {
			hal_error("[REPL UART0] buffer overflow. %d bytes was dropped.", huart->RxXferCount - left);
		}
		memcpy(repl_buffer + repl_buffer_tail, huart->pRxBuffPtr, left);
		repl_buffer_tail += left;
	}
}



//===========================================
static inline bool decode_utf8_from_buffer(int * code) {
	uint8_t * point = repl_buffer + repl_buffer_iter;
    int bytes = SizeOfUTF8Code[*point];
    if (repl_buffer_iter + bytes > repl_buffer_tail) {
        return false;
    }
    repl_buffer_iter += bytes;
    switch(bytes)
    {
    case 6:
        *code  = (*point & 0x01) << 30; ++point;
        *code += (*point & 0x3F) << 24; ++point;
        *code += (*point & 0x3F) << 18; ++point;
        *code += (*point & 0x3F) << 12; ++point;
        *code += (*point & 0x3F) << 6;  ++point;
        *code += (*point & 0x3F);
        break;
    case 5:
        *code  = (*point & 0x02) << 24; ++point;
        *code += (*point & 0x3F) << 18; ++point;
        *code += (*point & 0x3F) << 12; ++point;
        *code += (*point & 0x3F) << 6;  ++point;
        *code += (*point & 0x3F);
        break;
    case 4:
        *code  = (*point & 0x07) << 18; ++point;
        *code += (*point & 0x3F) << 12; ++point;
        *code += (*point & 0x3F) << 6;  ++point;
        *code += (*point & 0x3F);
        break;
    case 3:
        *code  = (*point & 0x0F) << 12; ++point;
        *code += (*point & 0x3F) << 6;  ++point;
        *code += (*point & 0x3F);
        break;
    case 2:
        *code  = (*point & 0x1F) << 6;  ++point;
        *code += (*point & 0x3F);
        break;
    case 1:
        *code = (*point);
        break;
    }
    return true;
}


//===========================================
// implement MPY HAL stdio
//===========================================
#include "xt804_mphal.h"
// implement functions which declared in <py/mphal.h>
// Receive single character
// int mp_hal_stdin_rx_chr(void) {
//     unsigned char c = 0;
   
//     // wait for RXNE
//     if (FifoDataLen() > 0) {
//         FifoRead(&c, 1);
//     }
    
//     return c;
// }
int mp_hal_stdin_rx_chr(void) {
	for (;;) {
		if (repl_buffer_iter < repl_buffer_tail) {
			int code = 0;
			if (decode_utf8_from_buffer(&code)) {
				return code;
			}
		}
		//GengYong: TODO: 
		mp_hal_delay_ms(1);
	}
    return 0;
}


void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    while (len--) {
        sendchar(*str++);
    }
}



