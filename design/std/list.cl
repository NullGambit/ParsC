module std.list 

import std.mem

// drop disables explicit copying 
struct List<T, A = #allocator> : Drop, Copy
{
    @readonly
    length: int
    @readonly
    capacity: int
    allocator: A
    ptr: *T

    Self(T... args)
    {
        reserve(args.length)

        $for item in args 
        {
            add(item)
        }
    }

    fn reserve(capacity: int)
    {
        var temp = allocator.alloc(capacity)

        memcpy(ptr, temp, length)

        self.capacity = capacity 

        allocator.free(ptr)

        ptr = temp
    }

    // called automatically when no longer in scope
    fn drop()
    {
        if ptr != nil 
        {
            allocator.free(ptr)
        }

        length = 0
        capacity = 0
        ptr = nil
    }

    fn op_index(index: int) => ptr[index]

    // will be called when slicing 
    fn op_slice(start, end: int) => ptr[start:end]

    fn copy(): Self
    {
        // $bit_copy is allowed even when copying is disabled
        var new_self = $bit_copy(self)

        new_self.ptr = new_self.allocator.alloc(new_self.capacity)

        memcpy(ptr, new_self.ptr, length)

        return new_self
    }

    fn ensure_size(length: int = 1)
    {
        if self.length + length >= capacity
        {
            const new_capacity = if capacity == 0 do length else capacity 
            reserve(new_capacity * 2)
        }
    }

    fn add(T item): *T
    {
        ensure_size()

        const index = length++

        ptr[index] = item

        return &ptr[index]
    }

    fn pop() => @ptr[length--]

    // reader writer specialization
    $if T is byte 
    {
        fn write<T>(value: const &&T)
        {
            import std.types 

            const bytes = get_ptr(value)
            const size = get_length(value)

            ensure_size(size)

            memcpy(ptr + length, bytes, size)

            length += size 
        }

        fn read<T>(start: int, size := sizeof(T)) => cast(T) ptr[start:start+size]
    }
}