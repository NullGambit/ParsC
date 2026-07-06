module std.list 

import std.mem

@readonly
struct List<T, A = #allocator>
{
    length: u64
    capacity: u64
    allocator: A
    ptr: ^T
}

impl List : Drop, Copy
{
    self(args: ...T)
    {
        self.reserve(args.length)

        $for item in args
        {
            self.add(item)
        }
    }

    fn reserve(capacity: u64)
    {
        var temp = self.allocator.alloc(capacity)

        memcpy(self.ptr, temp, self.length)

        self.capacity = capacity

        self.allocator.free(self.ptr)

        self.ptr = temp
    }

    // called automatically when no longer in scope
    fn drop()
    {
        if self.ptr != nil
        {
            self.allocator.free(self.ptr)
        }

        self.length = 0
        self.capacity = 0
        self.ptr = nil
    }

    fn op_index(index: u64) => self.ptr[self.index]

    // will be called when slicing
    fn op_slice(start, end: u64) => self.ptr[start:end]

    fn copy(): Self
    {
        // $bit_copy is allowed even when copying is disabled
        var new_self = $bit_copy(self)

        new_self.ptr = new_self.allocator.alloc(new_self.capacity)

        memcpy(self.ptr, new_self.ptr, self.length)

        return new_self
    }

    fn ensure_size(length: u64 = 1)
    {
        if self.length + length >= self.capacity
        {
            const new_capacity = if self.capacity == 0 do self.length else self.capacity
            self.reserve(new_capacity * 2)
        }
    }

    fn add(T item): *T
    {
        self.ensure_size()

        const index = self.length

        self.length += 1

        ptr[self.index] = item

        return &self.ptr[index]
    }

    fn pop(): T
    {
        var last = @self.ptr[self.length]

        self.length -= 1

        return last
    }

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

        fn read<T>(start: u64, size := sizeof(T)) => cast(T) ptr[start:start+size]
    }
}