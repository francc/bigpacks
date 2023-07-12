#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "bigpacks.h"

#define BUFFER_SIZE 64

void main()
{
    int32_t foo;
    float bar;
    bool baz;
    char qux[32];

    bp_type_t buffer[BUFFER_SIZE];
    bp_pack_t pack;

    bp_set_buffer(&pack, buffer, BUFFER_SIZE);

    // Packing
    bp_create_container(&pack, BP_MAP);
	    bp_put_string(&pack,  "foo");
    	bp_put_integer(&pack, 123);
	    bp_put_string(&pack,  "bar");
	    bp_put_float(&pack,   456.789f);
	    bp_put_string(&pack,  "baz");
	    bp_put_boolean(&pack, true);
	    bp_put_string(&pack,  "qux");
	    bp_put_string(&pack, "hello!");
    bp_finish_container(&pack);

    // Back to the start of the buffer
    bp_set_offset(&pack, 0);

    // Unpacking
    if(bp_next(&pack) && bp_is_map(&pack) && bp_open(&pack)) {
    	while(bp_next(&pack)) {
    		if(bp_match(&pack, "foo"))
    			foo = bp_get_integer(&pack);
    		else if(bp_match(&pack, "bar"))
    			bar = bp_get_float(&pack);
    		else if(bp_match(&pack, "baz"))
    			baz = bp_get_boolean(&pack);
    		else if(bp_match(&pack, "qux"))
    			bp_get_string(&pack, qux, sizeof(qux) / sizeof(bp_type_t));
    		else
    			bp_next(&pack);
    	}
    	bp_close(&pack);

    	printf("foo: %i, bar: %f, baz: %s, qux: %s\n", foo, bar, baz ? "true" : "false", qux);
    }
}
