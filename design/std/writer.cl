module std.writer 

duck Writer<T?>
{   
    // if T is provided find a method that exactly matches
    if try T 
    {
        fn write(T)
    }
    else 
    {
        // ? is used as a wildcard so any signature is valid
        fn write(?): ?
    }
}