module forge.engine.time 

// a static struct can never be initialized 
// all fields and methods of this struct are also static 
static struct Time 
{   
    // can only be modified within this package 
    // which includes all other modules under the forge.engine package
    @ReadOnly(.Package)
    delta: f32

    scale: f32 
}