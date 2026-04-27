module std.decoder 

import std.writer
import std.types
import std.encoding

struct Decoder<R: Reader = List<byte>>
{
    buffer: R
    cursor: int

    fn read<T: EncodableValue>(): T
    {
        var encoding_type = buffer.read<EncodedValue>(cursor)

        if encoding_type == .Str 
        {
            var length = buffer.read<int>(cursor)
            var str = buffer.read<str>(cursor, length)

            return str
        }

        var encoding_type = encoding_type
        {
            EncodedValue::Int => ,
            EncodedValue::Float,
            EncodedValue::Str
        }

        buffer.write(encoding_type)

        if $has_field<T>("length")
        {
            buffer.write(value.length)
        }

        buffer.write(value)

        cursor += sizeof(T)
    }
}

@Test 
{
    var encoder = Encoder{}

    encoder.write(10)
    encoder.write("hello")
    encoder.write([1, 2, 3])
}