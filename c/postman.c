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

#include "postman.h"

void postman_init(postman_t *postman)
{
    postman->registered_resources = 0;
}

bool postman_register_resource(postman_t *postman, const char *path, handler_t handler)
{
    if(postman->registered_resources < PM_MAX_RESOURCES) {
        postman->resources[postman->registered_resources].path = path;
        postman->resources[postman->registered_resources].handler = handler;
        postman->registered_resources += 1;
        return true;
    }
    else
        return false;
}

bp_length_t postman_handle_pack(postman_t *postman, bp_type_t *buffer, bp_length_t length, bp_length_t max_length)
{
    uint32_t i;
    uint32_t method_token = 0;
    uint32_t response_code = PM_404_Not_Found;
    bp_length_t response_length;

    bp_set_buffer(&postman->reader, buffer, length);
    bp_set_buffer(&postman->writer, buffer, max_length - 1);  // reserve one uint32_t for crc32

    if(!bp_next(&postman->reader) || !bp_is_integer(&postman->reader) || !(method_token = bp_get_integer(&postman->reader)))
        response_code = PM_400_Bad_Request;
    else if(!bp_next(&postman->reader) || !bp_set_offset(&postman->writer, bp_get_offset(&postman->reader)) || !bp_next(&postman->writer))
        response_code = PM_400_Bad_Request;
    else if(!bp_is_list(&postman->reader))
        response_code = PM_400_Bad_Request;
    else if(!bp_open(&postman->reader)) {
        if(method_token >> 24 == PM_GET) {
            bp_create_container(&postman->writer, BP_LIST);
            for(i = 0; i != postman->registered_resources; i++)
                bp_put_string(&postman->writer, postman->resources[i].path);
            bp_finish_container(&postman->writer);
            response_code = PM_205_Content;
        }
        else
            response_code = PM_405_Method_Not_Allowed;
    }
    else if(!bp_next(&postman->reader) || !bp_is_string(&postman->reader)) {
        response_code = PM_404_Not_Found;   // Integer paths not implemented yet
    }
    else {
        for(i = 0; i != postman->registered_resources; i++) {
            if(bp_equals(&postman->reader, postman->resources[i].path)) {
                response_code = (*postman->resources[i].handler)(method_token >> 24, &postman->reader, &postman->writer);
                break;
            }
        }
    }
    response_length = bp_get_offset(&postman->writer);
    bp_set_offset(&postman->writer, 0);
    bp_put_integer(&postman->writer, (response_code << 24) | (method_token & 0x00FFFFFF));
    return response_length;
}


