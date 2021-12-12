/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Robert Foss, Daniel Busch
 * Copyright (c) 2021 Geng Yong
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

//#include "py/mpconfig.h"

#include "xt804_apa102.h" 

#define NOP asm volatile (" nop \n\t")

static inline void _xt804_apa102_send_byte(const machine_pin_obj_t * clockPin, const machine_pin_obj_t * dataPin, uint8_t byte) {
    for (uint32_t i = 0; i < 8; i++) {
        if (byte & 0x80) {
            // set data pin high
            HAL_GPIO_WritePin(dataPin->gpio, dataPin->pin, GPIO_PIN_SET);
        } else {
            // set data pin low
            HAL_GPIO_WritePin(dataPin->gpio, dataPin->pin, GPIO_PIN_RESET);
        }

        // set clock pin high
        //GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, clockPinMask);
        HAL_GPIO_WritePin(clockPin->gpio, clockPin->pin, GPIO_PIN_SET);
        byte <<= 1;
        NOP;
        NOP;

        // set clock pin low
        //GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, clockPinMask);
        HAL_GPIO_WritePin(clockPin->gpio, clockPin->pin, GPIO_PIN_RESET);
        NOP;
        NOP;
    }
}

static inline void _xt804_apa102_send_colors(const machine_pin_obj_t * clockPin, const machine_pin_obj_t * dataPin, uint8_t *pixels, uint32_t numBytes) {
    for (uint32_t i = 0; i < numBytes / 4; i++) {
        _xt804_apa102_send_byte(clockPin, dataPin, pixels[i * 4 + 3] | 0xE0);
        _xt804_apa102_send_byte(clockPin, dataPin, pixels[i * 4 + 2]);
        _xt804_apa102_send_byte(clockPin, dataPin, pixels[i * 4 + 1]);
        _xt804_apa102_send_byte(clockPin, dataPin, pixels[i * 4]);
    }
}

static inline void _xt804_apa102_start_frame(const machine_pin_obj_t * clockPin, const machine_pin_obj_t * dataPin) {
    for (uint32_t i = 0; i < 4; i++) {
        _xt804_apa102_send_byte(clockPin, dataPin, 0x00);
    }
}

static inline void _xt804_apa102_append_additionial_cycles(const machine_pin_obj_t * clockPin, const machine_pin_obj_t * dataPin, uint32_t numBytes) {
    //GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, dataPinMask);
    HAL_GPIO_WritePin(dataPin->gpio, clockPin->pin, GPIO_PIN_SET);

    // we need to write some more clock cycles, because each led
    // delays the data by one edge after inverting the clock
    for (uint32_t i = 0; i < numBytes / 8 + ((numBytes / 4) % 2); i++) {
        //GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, clockPinMask);
        HAL_GPIO_WritePin(clockPin->gpio, clockPin->pin, GPIO_PIN_SET);
        NOP;
        NOP;

        //GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, clockPinMask);
        HAL_GPIO_WritePin(clockPin->gpio, clockPin->pin, GPIO_PIN_RESET);
        NOP;
        NOP;
    }
}

static inline void _xt804_apa102_end_frame(const machine_pin_obj_t * clockPin, const machine_pin_obj_t * dataPin) {
    for (uint32_t i = 0; i < 4; i++) {
        _xt804_apa102_send_byte(clockPin, dataPin, 0xFF);
    }
}

void xt804_apa102_write(const machine_pin_obj_t * clockPin, const machine_pin_obj_t * dataPin, uint8_t *pixels, uint32_t numBytes) {
    assert(clockPin != NULL);
    assert(dataPin != NULL);

    // start the frame
    _xt804_apa102_start_frame(clockPin, dataPin);

    // write pixels
    _xt804_apa102_send_colors(clockPin, dataPin, pixels, numBytes);

    // end the frame
    _xt804_apa102_append_additionial_cycles(clockPin, dataPin, numBytes);
    _xt804_apa102_end_frame(clockPin, dataPin);
}



