#!/usr/bin/python

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
