import ecs

struct App 
{
    world: World
    should_run: bool

    static fn new() => Self {}

    fn run()
    {
        world.call_system(InitSystem)

        while should_run 
        {
            world.call_system(UpdateSystem)
        }

        world.call_system(ShutdownSystem)
    }
}