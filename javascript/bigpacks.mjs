//  BigPacks - Copyright (c) 2022 Francisco Castro <http://fran.cc>
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

export default class BigPacks {
	static BP_FALSE    = 0x00000000;
	static BP_TRUE     = 0x10000000;
	static BP_NONE     = 0x20000000;
	static BP_INTEGER  = 0x40000000;
	static BP_FLOAT    = 0x50000000;
	static BP_LIST     = 0x80000000;
	static BP_MAP      = 0x90000000;
	static BP_STRING   = 0xC0000000;
	static BP_BINARY   = 0xD0000000;

	static BP_LENGTH_MAX      = 0x0FFFFFFF;
	static BP_LENGTH_MASK     = 0x0FFFFFFF;
	static BP_TYPE_MASK       = 0xF0000000;

	static pack(buffer, position, data, doubleFloats = false) {
		let view = new DataView(buffer);
		let bytes, length, last_position;

		switch (typeof data) {
			case "undefined":
				view.setUint32(position, BigPacks.BP_NONE, true);
				return position + 4;
			case "boolean":
				view.setUint32(position, data ? BigPacks.BP_TRUE : BigPacks.BP_FALSE, true);
				return position + 4;
			case "number":
				if (isFinite(data) && Math.floor(data) === data) {
					if (data >= -0x80000000 && data <= 0x7fffffff) {
						view.setUint32(position, BigPacks.BP_INTEGER | 1, true);
						view.setUint32(position + 4, data, true);
						return position + 8;
					}
					else if (data >= -0x8000000000000000 && data <= 0x7fffffffffffffff) {
						view.setUint32(position, BigPacks.BP_INTEGER | 2, true);
						view.setBigInt64(position + 4, data, true);
						return position + 12;
					}
					else
						throw new Error("Integer too big to fit Int64.");
				}
				else {
					if(!doubleFloats) {
						view.setUint32(position, BigPacks.BP_FLOAT | 1, true);
						view.setFloat32(position + 4, data, true);
						return position + 8;
					}
					else {
						view.setUint32(position, BigPacks.BP_FLOAT | 2, true);
						view.setFloat64(position + 4, data, true);
						return position + 12;
					}
				}
			case "string":
				bytes = new TextEncoder().encode(data);
				length = Math.floor((bytes.length + 1 + 3) / 4);
		        if (length <= BigPacks.BP_LENGTH_MAX) {
		        	view.setUint32(position, BigPacks.BP_STRING | length, true);
		        	for(let i = 0; i != length * 4; i++)
		        		view.setUint8(position + 4 + i, i < bytes.length ? bytes[i] : 0);
		        	return position + 4 + 4 * length;
		        }
		        else
					throw new Error("String too long.");
			case "object":
				if (data === null) {
					view.setUint32(position, BigPacks.BP_NONE, true);
					return position + 4;
				}
				else if (data instanceof ArrayBuffer) {
					bytes = new Uint8Array(data);
					length = Math.floor((bytes.length + 3) / 4);
			        if (length <= BigPacks.BP_LENGTH_MAX) {
			        	view.setUint32(position, BigPacks.BP_BINARY | length, true);
			        	for(let i = 0; i != length * 4; i++)
			        		view.setUint8(position + 4 + i, i < bytes.length ? bytes[i] : 0);
			        	return position + 4 + 4 * length;
			        }
			        else
						throw new Error("Binary too long.");
				}
				else if (Array.isArray(data) || 
					data instanceof Int8Array || data instanceof Uint8Array || data instanceof Uint8ClampedArray ||
					data instanceof Int16Array || data instanceof Uint16Array ||
					data instanceof Int32Array || data instanceof Uint32Array ||
					data instanceof Float32Array || data instanceof Float64Array) {

					last_position = position + 4;
					for (let i = 0; i < data.length; i++)
						last_position = BigPacks.pack(buffer, last_position, data[i], doubleFloats);
					view.setUint32(position, BigPacks.BP_LIST | ((last_position - position - 4) / 4), true);
		        	return last_position;
		        }
				else {
					last_position = position + 4;
					for (let key in data) {
						if (data[key] !== undefined) {
							last_position = BigPacks.pack(buffer, last_position, key, doubleFloats);
							last_position = BigPacks.pack(buffer, last_position, data[key], doubleFloats);
						}
					}
					view.setUint32(position, BigPacks.BP_MAP | ((last_position - position - 4) / 4), true);
		        	return last_position;
				}
			default:
				throw new Error("Unknown type " + (typeof data));
		}
	}

	static unpack(buffer, position) {
		if(buffer.length < 4)
			throw new Error("Can't unpack less than 4 bytes.");
		if(position >= buffer.length)
			throw new Error("Offset is out of buffer.");

		let view = new DataView(buffer);
		let type = (view.getUint32(position, true) & BigPacks.BP_TYPE_MASK) >>> 0;
		let content_length = ((view.getUint32(position, true) & BigPacks.BP_LENGTH_MASK) >>> 0) * 4;
		let obj, key, item, last_position, padding;

		switch(type) {
			case BigPacks.BP_NONE:
				obj = null;
				break;
			case BigPacks.BP_TRUE:
				obj = true;
				break;
			case BigPacks.BP_FALSE:
				obj = false;
				break;
			case BigPacks.BP_INTEGER:
				if(content_length == 4)
					obj = view.getInt32(position + 4, true);
				else if(content_length == 8) {
					obj = view.getBigInt64(position + 4, true);
					obj = obj < Number.MAX_SAFE_INTEGER && obj > Number.MIN_SAFE_INTEGER ? Number(obj) : obj;
				}
				else
					throw new Error("Integer too big.");
				break;
			case BigPacks.BP_FLOAT:
				if(content_length == 4)
					obj = view.getFloat32(position + 4, true);
				else if(content_length == 8)
					obj = view.getFLoat64(position + 4, true);
				else
					throw new Error("Float too big.");
				break;
			case BigPacks.BP_STRING:
		        last_position = position + 4 + content_length;
				padding = (view.getUint8(last_position - 1) ? 0:1) + (view.getUint8(last_position - 2) ? 0:1) +
						  (view.getUint8(last_position - 3) ? 0:1) + (view.getUint8(last_position - 4) ? 0:1);
				obj = new TextDecoder().decode(new Uint8Array(buffer, position + 4, content_length - padding));
				break;
			case BigPacks.BP_BINARY:
				obj = buffer.slice(position + 4, position + 4 + content_length);
				break;
			case BigPacks.BP_LIST:
				obj = new Array;
				last_position = position + 4;
		        while(last_position != position + 4 + content_length) {
		        	[item, last_position] = BigPacks.unpack(buffer, last_position);
		        	obj.push(item);
		        }
		        break;
			case BigPacks.BP_MAP:
				obj = {}
				last_position = position + 4;
		        while(last_position != position + 4 + content_length) {
		        	[key, last_position] = BigPacks.unpack(buffer, last_position);
		        	[item, last_position] = BigPacks.unpack(buffer, last_position); /// check if to out of map bounds
		        	obj[key] = item;
		        }
		        break;
		    default:
		    	throw new Error("The type '" + (typeof data) + "' cannot be serialized.");
		}
		return [obj, position + 4 + content_length];
	}
}
