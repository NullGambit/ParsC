// import all modules part of the engine package
import forge.engine.*

import std.io

struct TestService : Service 
{
    fn init(options: EngineOptions): error 
    {
        println("initted test service")
    }

    fn update()
    {
        println($"delta is {Time.delta}")
    }
}

// main that returns error will exit and log what
fn main(): error
{
    Engine::create_default()

    Engine.add_service<TestService>()?

    Engine.init({title: "my engine", window_size: {1920, 1080}})?

    Engine.start()
    Engine.shutdown()
}