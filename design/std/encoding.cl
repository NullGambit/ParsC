module std.encoding 

enum EncodedValue (u8)
{
    Int
    Float
    Str
}

duck EncodableValue<T>
{
    const InnerType = get_inner_type<T>() 

    InnerType == (str | int | float)
}