import std.collections

struct UpdateSystem 
struct InitSystem
struct ShutdownSystem 

alias Entity = u64

enum CommandOperation
{
    AddEntity,
    DestroyEntity,
    AddComponent(byte[]),
    RemoveComponent(byte[])
}

struct Command 
{
    entity: Entity
    operation: CommandOperation
}

struct Ctx 
{
}

alias SystemProc = fn (Ctx)

struct System 
{
    commands: List<Command>
    world: *World
    proc: SystemProc
}

struct World
{
    systems: Map<Type, System>

    // could just as easily be a template param but wanted to showcase this
    fn add_system(type: Type, proc: SystemProc): error
    {
        systems.add(type, System 
        {
            world: &self,
            proc: proc
        })?
    }

    fn call_system(type: Type): error
    {
        var system = systems.get(type)?
        
        system.proc({})
    }
}