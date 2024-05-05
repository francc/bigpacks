//  Postman - Copyright (c) 2022 Francisco Castro <http://fran.cc>
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

import { crc32 } from "./crc32.mjs";
import BigPacks from "./bigpacks.mjs";

export default class Postman {
    static PM_GET    = 0x01;
    static PM_POST   = 0x02;
    static PM_PUT    = 0x03;
    static PM_DELETE = 0x04;
    static PM_201_Created            = 0x21;
    static PM_202_Deleted            = 0x22;
    static PM_204_Changed            = 0x24;
    static PM_205_Content            = 0x25;
    static PM_400_Bad_Request        = 0x40;
    static PM_401_Unauthorized       = 0x41;
    static PM_403_Forbidden          = 0x43;
    static PM_404_Not_Found          = 0x44;
    static PM_405_Method_Not_Allowed = 0x45;
    static PM_413_Request_Entity_Too_Large     = 0x4D;

    static PM_RESPONSE_TEXT = {
        0x21: "201 Created",
        0x22: "202 Deleted",
        0x24: "204 Changed",
        0x25: "205 Content",
        0x40: "400 Bad Request",
        0x41: "401 Unauthorized",
        0x43: "403 Forbidden",
        0x44: "404 Not Found",
        0x45: "405 Method Not Allowed",
        0x4D: "413 Request Entity Too Large",
    };

    constructor(buffer_size = 8192, timeout = 10) {
        this.buffer_size = buffer_size;
        this.timeout = timeout;
        this.debug = false;
        this.running = false;
        this.port = null;
        this.token = 0;
        this.queue = [];
        this.rx_length = 0;
        this.rx_frame_buffer = new ArrayBuffer(buffer_size);
        this.rx_frame_array = new Uint8Array(this.rx_frame_buffer);
        this.tx_length = 0;
        this.tx_frame_buffer = new ArrayBuffer(buffer_size);
        this.tx_frame_array = new Uint8Array(this.tx_frame_buffer);
        this.tx_frame_view = new DataView(this.tx_frame_buffer);
    }

    async open(port) {
        if(this.port === null) {
            try {
                await port.open({ baudRate: 115200, bufferSize: this.buffer_size });
                this.port = port;

                await this.flush();
                this.loop();
                await this.send_null();
                await this.send_null();
                await this.send_null();
                return 0;
            } catch(error) {
                try {
                    await this.port.close();
                } catch {};
                this.port = null;
                this.running = false;
                console.log("postman.open error", error);
                return -1;
            }
        }
        else {
            console.log("Postman.open: port already open.")
            return 0;
        }
    }

    async flush() {
        try {
            const reader = this.port.readable.getReader();
            const timer = setTimeout(() => { 
                try {
                    reader.releaseLock();
                } catch {};
            }, 10);
            await reader.read();
            clearTimeout(timer);
            reader.releaseLock();
        } catch {};
    }

    async close() {
        if(this.port) {
            this.running = false;
            while (this.port && ((this.port.readable && this.port.readable.locked) || (this.port.writable && this.port.writable.locked))) {
              await this.sleep(250);
              // console.log("Postman.close: awaiting port release before closing");
            }
            await this.port.close();
            this.port = null;
            return 0;
        }
        else {
            console.log("Postman.close: port already closed.")
            return 0;
        }
    }

    async send(method, token, path, payload) {
        if(!this.port)
            return -1;

        this.tx_length = BigPacks.pack(this.tx_frame_buffer, 0, (method << 24 | token) >>> 0);
        this.tx_length = BigPacks.pack(this.tx_frame_buffer, this.tx_length, path);
        if(payload !== undefined)
            this.tx_length = BigPacks.pack(this.tx_frame_buffer, this.tx_length, payload);
        this.tx_frame_view.setUint32(this.tx_length, crc32(this.tx_frame_array.slice(0, this.tx_length)), true);
        this.tx_length += 4;

        if (this.debug)
            for (let i = 0; i != this.tx_length; i++)
                console.log("TX [", i, "] ", this.tx_frame_array[i].toString(16), String.fromCharCode(this.tx_frame_array[i]));

        const writer = this.port.writable.getWriter();
        await writer.write(new Uint8Array([ 0x7E ]));
        for(let i = 0; i != this.tx_length; i++) {
            if(this.tx_frame_array[i] == 0x7E)
                await writer.write(new Uint8Array([ 0x7D, 0x5E ]));
            else if(this.tx_frame_array[i] == 0x7D)
                await writer.write(new Uint8Array([ 0x7D, 0x5D ]));
            else
                await writer.write(this.tx_frame_array.slice(i, i+1));
        }
        await writer.write(new Uint8Array([ 0x7E ]));
        writer.releaseLock();
        return 0;
    }

    async send_null() {
        if(!this.port)
            return -1;
        const writer = this.port.writable.getWriter();
        await writer.write(new Uint8Array([ 0x7E ]));
        writer.releaseLock();
        return 0;
    }

    sleep (ms) {
        return new Promise(resolve => setTimeout(resolve, ms));
    }

    async loop() {
        let escape_flag = false;
        this.running = true;
        while (this.port && this.port.readable && this.running) {
            try {
                const reader = this.port.readable.getReader();
                const timer = setTimeout(() => { 
                    try {
                        reader.releaseLock();
                    } catch {};
                }, 250);
                const { value, done } = await reader.read();
                clearTimeout(timer);
                reader.releaseLock();
                // if (done) {
                //     this.running = false;
                //     console.log("Postman.loop: read() done!");
                //     break;
                // }
                if (value) {
                    // console.log("postman: reader.read() value:", value);
                    for(let i = 0; i != value.length; i++) {
                        if(value[i] == 0x7E) {
                            if (this.debug)
                                for(let j = 0; j != this.rx_length; j++)
                                    console.log("RX [", j, "] ", this.rx_frame_array[j].toString(16), String.fromCharCode(this.rx_frame_array[j]));
                            if(this.rx_length >= 12 && crc32(this.rx_frame_array.slice(0, this.rx_length)) == 0x2144df1c) {
                                this.rx_length -= 4;
                                let last_position = 0;
                                let response_token = null;
                                let path = null;
                                let payload = null;
                                let timestamp = null;
                                let identifier = null;
                                let signature = null;

                                [ response_token, last_position ] = BigPacks.unpack(this.rx_frame_buffer, 0);
                                if(last_position != this.rx_length)
                                    [ path, last_position ] = BigPacks.unpack(this.rx_frame_buffer, last_position);
                                if(last_position != this.rx_length)
                                    [ payload, last_position ] = BigPacks.unpack(this.rx_frame_buffer, last_position);
                                if(last_position != this.rx_length)
                                    [ timestamp, last_position ] = BigPacks.unpack(this.rx_frame_buffer, last_position);
                                if(last_position != this.rx_length)
                                    [ identifier, last_position ] = BigPacks.unpack(this.rx_frame_buffer, last_position);
                                if(last_position != this.rx_length)
                                    [ signature, last_position ] = BigPacks.unpack(this.rx_frame_buffer, last_position);                            

                                this.queue.push([ response_token >> 24, response_token & 0x00FFFFFF, path, payload, timestamp, identifier, signature ]);
                            }
                            else if(this.rx_length)
                                console.log("Postman.loop: Frame receiving failed, bad CRC.");
                            this.rx_length = 0;
                            escape_flag = false;
                        }
                        else if (value[i] == 0x7D) {
                            escape_flag = true;
                        }
                        else {
                            this.rx_frame_array[this.rx_length] = escape_flag ? value[i] | 32 : value[i];
                            this.rx_length += 1;
                            escape_flag = false;
                        }
                    }
                }
            } catch (error) {
                if (error instanceof TypeError) {
                    continue;
                }
                else if (error instanceof DOMException) {
                    this.running = false;
                    console.log("Postman.loop: readLoop port lost, exiting...");
                    return;
                }
                else {
                    console.log("Postman.loop: ", error);
                }
            }
        }
    }

    async method(method, path, data) {
        this.queue = [];
        if(!(await this.send(method, this.token, path, data))) {
            for(let i = 0; i < this.timeout * 10; i++) {
                await new Promise(r => setTimeout(r, 100));
                if(this.queue.length > 0) {
                    let response = this.queue.pop();
                    if(response.length > 1 && response[1] != this.token)
                        return [ -2, null, null ];
                    this.token = (this.token + 1) % 0xFFFFFFFF;
                    return response;
                }
            }
            console.log("method timeout!");
        }
        return [ -1, null, null ]
    }

    async get(path, data) {
        return await this.method(Postman.PM_GET, path, data);
    }

    async put(path, data) {
        return await this.method(Postman.PM_PUT, path, data);
    }

    async post(path, data) {
        return await this.method(Postman.PM_POST, path, data);
    }

    async delete(path, data) {
        return await this.method(Postman.PM_DELETE, path, data);
    }
}



