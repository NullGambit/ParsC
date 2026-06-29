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
import core.stdc.stdio
import core.stdc.stdlib

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
}
```