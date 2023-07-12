# BigPacks

**BigPacks** is a data serialization format designed for 32-bit microcontrollers.
It aims at being minimal and fast, without obsessing over achieving the
smallest possible binary representation.

This is the evolution of another serialization format for 8 and 16-bit
microcontrollers called [TinyPacks](https://github.com/francc/tinypacks),
that is where the name comes from.

## Specs

**Easy to traverse**. This allows fast in-place parsing using static memory allocation,
and skipping nested elements without parsing them.

**Small encoder/decoder memory footprint**. Uses 68 bytes of RAM with the
default configuration that allows up to 4 element nesting levels (yes, bytes).
Compiled for the ESP32, it uses about 1700 bytes of flash.

**Easy translation to and from JSON**. Any valid JSON can be translated to BigPacks,
but binary objects in BigPacks do not have a direct equivalent in JSON.

**Similar encoded size to JSON**. Data encoded with BigPacks can be bigger or smaller
than the equivalent JSON, depending on the mix of data types and value ranges,
but they are about the same for real-world usage.

**Update in-place**. There is limited support for update-in-place for some
data types, that could help reduce memory usage by avoiding the need for separate
buffers.

**32-bit aligned**. All the data structures in BigPacks are 32-bit aligned, that is why it is
not so tight, but this optimizes memory access on 32-bit architectures.

**Little-endian only**. Why? Because it is 2023 and it seems little-endian won
the endianess war 🤷‍♂️.

## Data types

    None        Null value.
    Boolean     Boolean true or false value.
    Integer     Signed integer 32 or 64 bits long.
    Float       Floating point number 32 or 64 bits long.
    String      Zero-ended string of characters encoded as UTF-8.
    Binary      Sequence of 32-bit words.
    List        Sequence of values.
    Map         Sequence of key-value pairs.

## Serialization format

	packet 		:= element...
	element 	:= false | true | none | integer | float | list | map | string | binary
	false 		:= 0x00000000
	true 		:= 0x10000000
	none 		:= 0x20000000
	integer 	:= integer_32 | integer_64
	integer_32  := 0x40000001 c_int32_t
	integer_64  := 0x40000002 c_int64_t
	float 		:= float_32 | float_64
	float_32    := 0x50000001 c_float
	float_32    := 0x50000002 c_double
	list    	:= 0x8nnnnnnn list_data		// where n is the length of list_data expressed in 32-bit words
	list_data	:= element...
	map 		:= 0x9nnnnnnn map_data		// where n is the length of map_data expressed in 32-bit words
	map_data	:= (element element)...
	string  	:= 0xCnnnnnnn string_data   //  where n is the length of string_data expressed in 32-bit words
											// including a required zero terminator and zero padding filling the
											// remainder if any of the last 32-bit word.
	string_data := c_char... 0x00 0x00...
	binary 		:= 0xDnnnnnnn binary_data	//  where n is the length of binary_data expressed in 32-bit words
	binary_data := c_uint32_t...

	**Syntax**
	<space> := sequence of objects
	...		:= repetition of zero or more objects
	|		:= only one of the alternatives
	()  	:= the following operator applies to all enclosed objects
	c_		:= C data type encoded as little-endian
	0x 		:= hexadecimal number encoded as little-endian


## Serialization examples

    Python                                      BigPacks
    ------                                      ---------
    False                                       00 00 00 00

    True                                        00 00 00 10

    None                                        00 00 00 20

    1234										01 00 00 40 d2 04 00 00

    -5678                                       01 00 00 40 d2 e9 ff ff

    123.4567									01 00 00 50 79 e9 f6 42

    "hello world!"								04 00 00 c0 68 65 6c 6c
    											6f 20 77 6f 72 6c 64 21
    											00 00 00 00

    b"\x01\x02\x03"								01 00 00 d0 01 02 03 00

    [1, 2, 3]									06 00 00 80 01 00 00 40
    											01 00 00 00 01 00 00 40
    											02 00 00 00 01 00 00 40
    											03 00 00 00

    [4, True, "fun"]							05 00 00 80 01 00 00 40
    											04 00 00 00 00 00 00 10
    											01 00 00 c0 66 75 6e 00

    {"a": 1, "b": False, "c": "foo"}			0b 00 00 90 01 00 00 c0
    											61 00 00 00 01 00 00 40
    											01 00 00 00 01 00 00 c0
    											62 00 00 00 00 00 00 00
    											01 00 00 c0 63 00 00 00
    											01 00 00 c0 66 6f 6f 00

    {"foo": [1, 2], "bar": {True: 3, False: 4}}	10 00 00 90 01 00 00 c0
    											66 6f 6f 00 04 00 00 80
    											01 00 00 40 01 00 00 00
    											01 00 00 40 02 00 00 00
    											01 00 00 c0 62 61 72 00
    											06 00 00 90 00 00 00 10
    											01 00 00 40 03 00 00 00
    											00 00 00 00 01 00 00 40
    											04 00 00 00


## Postman

A RESTish library based on BigPacks is also included in this project. It allows you to implement REST-like APIs over serial ports or other transports. It is being used in [SensorWatcher](https://movoki.com/setup) for communication between the Javascript code running in a web browser and the C code running in a ESP32, and also as an API over HTTP to upload measurements to backends.

## Usage

### C

Add the files *bigpacks.c* and *bigpacks.h* to your project.

```C
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
```

### Python

Add the file *bigpacks.py* to your project.

```python
import bigpacks

data = {
	"foo": 123,
	"bar": 456.789,
	"baz": True,
	"qux": "hello!"
}

packed = bigpacks.pack(data)
(unpacked, remainder) = bigpacks.unpack(packed)

print(unpacked)
```

### Javascript

Add the file *bigpacks.mjs* to your project.

```javascript
import BigPacks from "./bigpacks.mjs";

let length = 0;
let unpacked;
let buffer = new ArrayBuffer(256);
let data = {
	"foo": 123,
	"bar": 456.789,
	"baz": true,
	"qux": "hello!"
}

length = BigPacks.pack(buffer, 0, data);
[ unpacked, length ] = BigPacks.unpack(buffer, 0);

console.log(unpacked);
```
