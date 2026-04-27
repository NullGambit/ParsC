module std.types 

// gets either the size of T or its length field
fn get_length<T>(value: &&T): int
{
    $if $has_field<T>("length")
    {
        return value.length 
    }

    return sizeof(value)
}

// gets either a pointer to value or its ptr field
fn get_ptr<T>(value: &&T): *T
{
    $if $has_field<T>("ptr")
    {
        return value.ptr 
    }

    return &value
}