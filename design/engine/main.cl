import std.io 

import engine.app

fn init_demo(ctx: Ctx)
{
    println("Hello")
}

fn main()
{
    var app = App::new()

    app.add_system(InitSystem, init_demo)

    app.run()
}