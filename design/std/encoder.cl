module std.encoder 

import std.writer
import std.types
import std.encoding

duck EncodableValue<T>
{
    const InnerType = get_inner_type<T>() 

    InnerType == (str | int | float)
}

struct Encoder<W: Writer = List<byte>>
{
    buffer: W

    fn write<T: EncodableValue>(value: T)
    {
        var encoding_type = match get_inner_type<T>()
        {
            int => EncodedValue::Int,
            float => EncodedValue::Float,
            str => EncodedValue::Str
        }

        buffer.write(encoding_type)

        if $has_field<T>("length")
        {
            buffer.write(value.length)
        }

        buffer.write(value)
    }
}

@Test 
{
    var encoder = Encoder{}

    encoder.write(10)
    encoder.write("hello")
    encoder.write([1, 2, 3])
}