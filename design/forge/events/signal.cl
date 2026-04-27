module forge.events.signal 

import std.collections
import std.function

struct Signal<A...>
{
    // function can wrap function pointers and captured closures
    alias ConnectionFn = Function<void, A>

    private
    {
        struct Connection
        {
            f: ConnectionFn
            previous_free_slot := uint.max
        }

        connections: List<Connection>
        next_free_slot := uint.max
    }

    fn connect(f: ConnectionFn): uint
    {
        if next_free_slot != uint.max
        {
            var free_connection = &connections[next_free_slot]

            var id = next_free_slot

            next_free_slot = free_connection.previous_free_slot

            free_connection.previous_free_slot = uint.max

            return id
        }

        connections.add(Connection {f})

        return connections.length - 1
    }

    fn op_op_assign<Op: str>(f: ConnectionFn) if Op == "+" => connect(f)
    fn op_op_assign<Op: str>(id: uint) if Op == "-" => disconnect(id)

    fn disconnect(id: uint)
    {
        if id >= connections.length
        {
            return;
        }

        if next_free_slot != uint.max
        {
            connections[next_free_slot].previous_free_slot = next_free_slot
        }

        connections[id].f = nil

        next_free_slot = id
    }

    fn op_call(args: A) => emit(args)

    fn emit(A args)
    {
        for c in connections where c.f != nil
        {
            c.f(args)
        }
    }
}

utest "signals"
{
    var signal = Signal<int, str>{}

    var closure = (n, s) => println($"{n} {s}")

    signal += closure 

    signal(10, "hello") 
}


// alternative syntax 

opaque Signal<A...>
{   
    alias ConnectionFn = Function<fn (A)>
    
    struct Connection
    {
        f: ConnectionFn
        previous_free_slot := u32.max
    }

    connections: List<Connection>
    next_free_slot := u32.max

    
}

impl Signal
{
    fn connect(s, f: ConnectionFn): uint
    {
        if s.next_free_slot != uint.max
        {
            var free_connection = &s.connections[s.next_free_slot]

            var id = s.next_free_slot

            s.next_free_slot = free_connection.previous_free_slot

            free_connection.previous_free_slot = uint.max

            return id
        }

        s.connections.add(Connection {f})

        return s.connections.length - 1
    }

    fn op_op_assign<Op: str>(f: ConnectionFn) if Op == "+" => connect(f)
    fn op_op_assign<Op: str>(id: uint) if Op == "-" => disconnect(id)

    fn disconnect(id: uint)
    {
        if id >= connections.length
        {
            return;
        }

        if next_free_slot != uint.max
        {
            connections[next_free_slot].previous_free_slot = next_free_slot
        }

        connections[id].f = nil

        next_free_slot = id
    }

    fn op_call(args: A) => emit(args)

    fn emit(A args)
    {
        for c in connections where c.f != nil
        {
            c.f(args)
        }
    }
}