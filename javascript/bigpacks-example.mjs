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
