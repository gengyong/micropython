#include <unistd.h>
#include "py/mpconfig.h"
#include "py/ringbuf.h"
#include "py/stream.h"

#include "wm_hal.h"
#include "hal_common.h"
#include "hal_uart0_core.h"
//#include "hal_uart0_fifo.h"
#include "libc_port.h"
#include "utf8_map.h"

// ================ configuration section ===
// define REPL(listen on uart0) input buffer size
//#define REPL_INPUT_BUFFER_SIZE  (256)
// ==========================================
// static int16_t repl_buffer_tail = 0;
// static int16_t repl_buffer_iter = 0;
// static uint8_t repl_buffer[REPL_INPUT_BUFFER_SIZE];

STATIC uint8_t stdin_ringbuf_array[260];
ringbuf_t stdin_ringbuf = {stdin_ringbuf_array, sizeof(stdin_ringbuf_array), 0, 0};

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
		//repl_buffer_tail = 0;
		//repl_buffer_iter = 0;
        stdin_ringbuf.iget = 0;
        stdin_ringbuf.iput = 0;
	    HAL_UART_Receive_IT(&huart, uart0_buffer, 0);
	} else {
        TPANIC("UART0: init failed.");
    }
}

void HAL_UART0_DeInit() {
	HAL_UART_DeInit(&huart);
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    for (int i = 0; i < huart->RxXferCount; i++) {
        if (ringbuf_put(&stdin_ringbuf, huart->pRxBuffPtr[i]) < 0) {
            TERROR("[REPL UART0] buffer overflow. %d bytes was dropped.", huart->RxXferCount - i);
            return;
        } 
    }

}



//===========================================
static inline bool decode_utf8_from_buffer(int * code) {
    int point = ringbuf_peek(&stdin_ringbuf);
    if (point > 0) {
	    int bytes = SizeOfUTF8Code[point];
        if (ringbuf_avail(&stdin_ringbuf) >= bytes) {
            ringbuf_get(&stdin_ringbuf);
            switch(bytes)
            {
            case 6:
                *code  = (point & 0x01) << 30; point = ringbuf_get(&stdin_ringbuf);
                *code += (point & 0x3F) << 24; point = ringbuf_get(&stdin_ringbuf);
                *code += (point & 0x3F) << 18; point = ringbuf_get(&stdin_ringbuf);
                *code += (point & 0x3F) << 12; point = ringbuf_get(&stdin_ringbuf);
                *code += (point & 0x3F) << 6;  point = ringbuf_get(&stdin_ringbuf);
                *code += (point & 0x3F);
                break;
            case 5:
                *code  = (point & 0x02) << 24; point = ringbuf_get(&stdin_ringbuf);
                *code += (point & 0x3F) << 18; point = ringbuf_get(&stdin_ringbuf);
                *code += (point & 0x3F) << 12; point = ringbuf_get(&stdin_ringbuf);
                *code += (point & 0x3F) << 6;  point = ringbuf_get(&stdin_ringbuf);
                *code += (point & 0x3F);
                break;
            case 4:
                *code  = (point & 0x07) << 18; point = ringbuf_get(&stdin_ringbuf);
                *code += (point & 0x3F) << 12; point = ringbuf_get(&stdin_ringbuf);
                *code += (point & 0x3F) << 6;  point = ringbuf_get(&stdin_ringbuf);
                *code += (point & 0x3F);
                break;
            case 3:
                *code  = (point & 0x0F) << 12; point = ringbuf_get(&stdin_ringbuf);
                *code += (point & 0x3F) << 6;  point = ringbuf_get(&stdin_ringbuf);
                *code += (point & 0x3F);
                break;
            case 2:
                *code  = (point & 0x1F) << 6;  point = ringbuf_get(&stdin_ringbuf);
                *code += (point & 0x3F);
                break;
            case 1:
                *code = (point);
                break;
            }
            return true;
        }
    }
    return false;
}


//===========================================
// implement MPY HAL stdio
//===========================================
#include "mphalport.h"
#include "py/runtime.h"
uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags) {
    uintptr_t ret = 0;
    if ((poll_flags & MP_STREAM_POLL_RD) && stdin_ringbuf.iget != stdin_ringbuf.iput) {
        ret |= MP_STREAM_POLL_RD;
    }
    return ret;
}

int mp_hal_stdin_rx_chr(void) {
	for (;;) {
		if (stdin_ringbuf.iget != stdin_ringbuf.iput) {
			int code = 0;
			if (decode_utf8_from_buffer(&code)) {
                //if (code == mp_interrupt_char) {
                if (code == 3) {
                    printf("INTERRRRRRRRUUUUUPPPPPPTT!\r\n");
                    mp_sched_keyboard_interrupt();
                } else {
				    return code;
                }
			}
		}
		//GengYong: TODO: 
		MICROPY_EVENT_POLL_HOOK
	}
    return 0;
}

void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    while (len--) {
        sendchar(*str++);
    }
}



