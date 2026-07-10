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
    
    // local imports
    import core.stdc.stdlib
    
    // unrestricted memory access
    var ptr = cast(^i32)malloc(sizeof(i32))
     
    if ptr == nil 
    {
        printf("pointer is nil\n")
        return
    }
    
    ^ptr = 10
    
    printf("%d %p\n", ^ptr, ptr)
}
```

### Structs

```rs
import core.stdc.stdio

struct User
{
    name: str
    score: u32
}

fn init_user(user: ^User): bool
{
    if user == nil
    {
        return false
    }
    
    // automatic dereferncing
    user.name = "unnamed user"
    user.score = u32.min

    return true
}

fn fatal(message: str, code: i32 = -1)
{
    import core.stdc.stdlib : exit

    printf("[FATAL] %s\n", message)

    exit(code)
}

fn main()
{
    var user = User{}

    var ok = init_user(&user)

    if !ok
    {
        fatal("could not init user", code: 20)
    }

    printf("user: name = %s, score = %d\n", user.name, user.score)
}

```