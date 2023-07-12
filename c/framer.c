//  Framer - Copyright (c) 2022 Francisco Castro <http://fran.cc>
//  SPDX short identifier: MIT
//
//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.

#include "framer.h"

bp_length_t framer_get_length(framer_t *framer) { return framer->length; };
void        framer_set_length(framer_t *framer, bp_length_t value) { framer->length = value; };

bool framer_get_state(framer_t *framer) { return framer->state; };
void framer_set_state(framer_t *framer, bool value) { 
    framer->state = value;
    framer->index = 0;
    framer->crc = 0;
    framer->start = true;
};


uint32_t framer_crc32(uint32_t crc, uint8_t value)
{
    const uint32_t lut[16] = {
        0x00000000,0x1DB71064,0x3B6E20C8,0x26D930AC,0x76DC4190,0x6B6B51F4,0x4DB26158,0x5005713C,
        0xEDB88320,0xF00F9344,0xD6D6A3E8,0xCB61B38C,0x9B64C2B0,0x86D3D2D4,0xA00AE278,0xBDBDF21C
    };

    crc = ~crc;
    crc = lut[(crc ^  value      ) & 0x0F] ^ (crc >> 4);
    crc = lut[(crc ^ (value >> 4)) & 0x0F] ^ (crc >> 4);
    return ~crc;
}

void framer_set_buffer(framer_t *framer, uint8_t *pack_buffer, bp_length_t pack_max_length) {
    framer->buffer = pack_buffer;
    framer->max_length = pack_max_length;
    framer->state = FRAMER_RECEIVING;
    framer->crc = 0;
    framer->length = 0;
    framer->index = 0;
    framer->escape = false;
    framer->start = true;
}

bool framer_put_received_byte(framer_t *framer, uint8_t value) {
    bool valid_frame = false;
    if(framer->state == FRAMER_RECEIVING) {
        if(value == 0x7E) {
            valid_frame = framer->index > 2 && framer->crc == 0x2144df1c;
            framer->length = framer->index - 2;
            framer_set_state(framer, FRAMER_RECEIVING);
        }
        else if(value == 0x7D)
            framer->escape = true;
        else {
            value = framer->escape ? value ^ 0x20 : value;
            framer->escape = false;
            framer->crc = framer_crc32(framer->crc, value);
            if(framer->index < framer->max_length)
                framer->buffer[framer->index++] = value;
        }
    }
    return valid_frame;
}

uint8_t framer_get_byte_to_send(framer_t *framer) {
    if(framer->state == FRAMER_SENDING) {
        if(framer->start) {
            framer->start = false;
            return 0x7E;
        }

        if(framer->index == framer->length + 4) {
            framer_set_state(framer, FRAMER_RECEIVING);
            return 0x7E;
        }
        else if(framer->index == framer->length) {
            framer->buffer[framer->index + 0] = framer->crc & 0xFF;
            framer->buffer[framer->index + 1] = framer->crc >> 8 & 0xFF;
            framer->buffer[framer->index + 2] = framer->crc >> 16 & 0xFF;
            framer->buffer[framer->index + 3] = framer->crc >> 24 & 0xFF;
        }

        if(framer->escape) {
            framer->escape = false;
            framer->crc = framer_crc32(framer->crc, framer->buffer[framer->index]);
            return framer->buffer[framer->index++] ^ 0x20;
        }
        else if(framer->buffer[framer->index] == 0x7E || framer->buffer[framer->index] == 0x7D) {
            framer->escape = true;
            return 0x7D;
        }
        else {
            framer->crc = framer_crc32(framer->crc, framer->buffer[framer->index]);
            return framer->buffer[framer->index++];
        }
    }
    else
        return 0x7E;
};
