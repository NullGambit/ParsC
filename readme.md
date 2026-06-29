# Pars

Pars is a fast and expressive systems programming language that's rust without handbrakes.

Currently, the language is extremely experimental and WIP.

## Examples

### Hello world
```rs
import core.std.stdio

fn main()
{
    printf("hello world!\n)
}
```

### Control Flow and functions
```rs
// built in c interop
import core.stdc.stdio
import core.stdc.stdlib

// | | abs operator and ** power operator
fn do_math(x: i32) => | 1 - 2 ** -x |

fn main()
{
    var limit = 10
    
    for i in 0..limit
    {
        printf("#%d\n", i)
    }

    if limit > 5
    {
        printf("%d is bigger than 5\n", limit)
    }
    else
    {
        printf("%d is smaller than 5\n", limit)
    }

    var y = do_math(32)

    printf("math result = %d\n", y)
    
    // unrestricted memory access
    var ptr = cast(i32)malloc(sizeof(i32))
     
    if ptr == nil 
    {
        printf("pointer is nil\n")
        return
    }
    
    ^ptr = 10
    
    printf("%d %p\n", ^ptr, ptr)
}
```