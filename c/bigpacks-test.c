#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "bigpacks.h"

void main()
{
    uint32_t pack_buffer[70] = {};
    bp_pack_t pack;

    uint32_t binary[] = { 0x12345678, 0x87654321, 0x44444444, 0xffffffff };
    bp_set_buffer(&pack, pack_buffer, sizeof(pack_buffer) / 4);

    bp_put_none(&pack);

    bp_put_boolean(&pack, true);
    bp_put_boolean(&pack, false);

    bp_put_integer(&pack, 0);
    bp_put_integer(&pack, 23);
    bp_put_integer(&pack, -1234567890);
    bp_put_big_integer(&pack, 12345678987654321LL);
    bp_put_big_integer(&pack, -12345678987654321LL);

    bp_put_float(&pack,   123.456f);
    bp_put_float(&pack,   -987.6543f);
    bp_put_double(&pack, 1234567898.76543);
    bp_put_double(&pack, -1234567898.76543);

    bp_put_string(&pack,  "hello world");
    bp_put_binary(&pack,  (uint32_t *)&binary, sizeof(binary) / 4);

    bp_create_container(&pack, BP_LIST);
    bp_put_boolean(&pack, false);
    bp_put_integer(&pack, 1337);
    bp_put_float(&pack,   12.34f);
    bp_put_string(&pack,  "foo bar");
    bp_finish_container(&pack);

    assert(bp_create_container(&pack, BP_MAP));
    assert(bp_put_string(&pack,  "key1"));
    assert(bp_put_boolean(&pack, false));
    assert(bp_put_string(&pack,  "key2"));
    assert(bp_put_integer(&pack, 1337));
    assert(bp_put_string(&pack,  "key3"));
    assert(bp_put_float(&pack,   12.34f));
    assert(bp_put_string(&pack,  "key4"));
    assert(bp_put_string(&pack,  "foo bar"));
    assert(!bp_put_string(&pack, "very long string to exceed buffer size"));
    assert(bp_finish_container(&pack));

    assert(bp_set_offset(&pack, 0));

    for(int i=0; i != sizeof(pack_buffer) / 4; i++)
        printf("%02i: %08x\n", i, pack_buffer[i]);

    assert(bp_next(&pack));

    assert(bp_is_none(&pack));
    assert(bp_has_next(&pack));
    assert(bp_next(&pack));

    assert(bp_is_boolean(&pack));
    assert(bp_get_boolean(&pack));
    assert(bp_has_next(&pack));
    assert(bp_next(&pack));

    // printf("\n\n%08x\n", *pack.cursor->element_start);

    assert(bp_is_boolean(&pack));
    assert(!bp_get_boolean(&pack));
    assert(bp_has_next(&pack));
    assert(bp_next(&pack));

    assert(bp_is_integer(&pack));
    assert(!bp_is_float(&pack));
    assert(bp_is_number(&pack));
    assert(bp_get_integer(&pack) == 0);
    assert(bp_get_float(&pack) == 0.0f);
    assert(bp_has_next(&pack));
    assert(bp_next(&pack));

    assert(bp_is_integer(&pack));
    assert(!bp_is_float(&pack));
    assert(bp_is_number(&pack));
    assert(bp_get_integer(&pack) == 23);
    assert(bp_get_float(&pack) == 23.0f);
    assert(bp_has_next(&pack));
    assert(bp_next(&pack));

    assert(bp_is_integer(&pack));
    assert(!bp_is_float(&pack));
    assert(bp_is_number(&pack));
    assert(bp_get_integer(&pack) == -1234567890);
    assert(!(bp_get_float(&pack) == -1234567890.0));  // do not fit in single precision float
    assert(bp_has_next(&pack));
    assert(bp_next(&pack));

    assert(bp_is_integer(&pack));
    assert(!bp_is_float(&pack));
    assert(bp_is_number(&pack));
    assert(bp_get_big_integer(&pack) == 12345678987654321LL);
    assert(!(bp_get_float(&pack) == 12345678987654321.0));  // do not fit in single precision float
    assert(bp_has_next(&pack));
    assert(bp_next(&pack));

    assert(bp_is_integer(&pack));
    assert(!bp_is_float(&pack));
    assert(bp_is_number(&pack));
    assert(bp_get_big_integer(&pack) == -12345678987654321LL);
    assert(!(bp_get_float(&pack) == -12345678987654321.0));  // do not fit in single precision float
    assert(bp_has_next(&pack));
    assert(bp_next(&pack));

    assert(bp_is_float(&pack));
    assert(!bp_is_integer(&pack));
    assert(bp_is_number(&pack));
    assert(bp_get_float(&pack) == 123.456f);
    assert(bp_get_integer(&pack) == 123);
    assert(bp_has_next(&pack));
    assert(bp_next(&pack));

    assert(bp_is_float(&pack));
    assert(!bp_is_integer(&pack));
    assert(bp_is_number(&pack));
    assert(bp_get_float(&pack) == -987.6543f);
    assert(bp_get_integer(&pack) == -987);
    assert(bp_has_next(&pack));
    assert(bp_next(&pack));

    assert(bp_is_float(&pack));
    assert(!bp_is_integer(&pack));
    assert(bp_is_number(&pack));
    assert(bp_get_double(&pack) == 1234567898.76543);
    assert(bp_get_big_integer(&pack) == 1234567898);
    assert(bp_has_next(&pack));
    assert(bp_next(&pack));

    assert(bp_is_float(&pack));
    assert(!bp_is_integer(&pack));
    assert(bp_is_number(&pack));
    assert(bp_get_double(&pack) == -1234567898.76543);
    assert(bp_get_big_integer(&pack) == -1234567898);
    assert(bp_has_next(&pack));
    assert(bp_next(&pack));

    char string_buffer[64];
    assert(bp_is_string(&pack));
    assert(!bp_is_binary(&pack));
    assert(bp_is_block(&pack));
    assert(!bp_is_integer(&pack));
    assert(!bp_is_float(&pack));
    assert(!bp_is_number(&pack));
    assert(!bp_is_container(&pack));
    assert(bp_get_string(&pack, string_buffer, sizeof(string_buffer) / 4) == 3);
    assert(!strcmp(string_buffer, "hello world"));
    assert(bp_equals(&pack, "hello world"));
    assert(bp_match(&pack, "hello world"));

    uint32_t binary_buffer[4];
    assert(bp_is_binary(&pack));
    assert(!bp_is_string(&pack));
    assert(bp_is_block(&pack));
    assert(!bp_is_integer(&pack));
    assert(!bp_is_float(&pack));
    assert(!bp_is_number(&pack));
    assert(!bp_is_container(&pack));
    assert(bp_get_binary(&pack, binary_buffer, sizeof(string_buffer) / 4) == 4);
    assert(binary_buffer[0] == 0x12345678);
    assert(binary_buffer[1] == 0x87654321);
    assert(binary_buffer[2] == 0x44444444);
    assert(binary_buffer[3] == 0xffffffff);

    assert(bp_next(&pack));

    assert(bp_is_list(&pack));
    assert(bp_is_container(&pack));
    assert(!bp_is_map(&pack));
    assert(!bp_is_boolean(&pack));
    assert(!bp_is_number(&pack));
    assert(!bp_is_block(&pack));
    assert(bp_open(&pack));
    assert(bp_has_next(&pack));
    assert(bp_next(&pack));
    assert(bp_is_boolean(&pack));
    assert(bp_get_boolean(&pack) == false);
    assert(bp_next(&pack));
    assert(bp_is_integer(&pack));
    assert(bp_get_integer(&pack) == 1337);
    assert(bp_next(&pack));
    assert(bp_is_float(&pack));
    assert(bp_get_float(&pack) == 12.34f);
    assert(bp_next(&pack));
    assert(bp_is_string(&pack));
    assert(bp_equals(&pack, "foo bar"));
    assert(bp_close(&pack));

    assert(bp_next(&pack));

    assert(bp_is_map(&pack));
    assert(bp_is_container(&pack));
    assert(!bp_is_list(&pack));
    assert(!bp_is_boolean(&pack));
    assert(!bp_is_number(&pack));
    assert(!bp_is_block(&pack));
    assert(bp_open(&pack));
    assert(bp_has_next(&pack));
    assert(bp_next(&pack));
    assert(bp_match(&pack, "key1"));
    assert(bp_get_boolean(&pack) == false);
    assert(bp_next(&pack));
    assert(bp_match(&pack, "key2"));
    assert(bp_get_integer(&pack) == 1337);
    assert(bp_next(&pack));
    assert(bp_match(&pack, "key3"));
    assert(bp_get_float(&pack) == 12.34f);
    assert(bp_next(&pack));
    assert(bp_match(&pack, "key4"));
    assert(bp_equals(&pack, "foo bar"));
    assert(bp_close(&pack));

}