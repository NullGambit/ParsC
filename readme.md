# Pars

pars is a systems programming language that allows the user to write unrestricted low level code as well as expressive high level code.

pars takes inspiration from multiple languages and offers truly multiparadigm programming.

currently pars interms of features is just a more powerful version of C but in the future it will have traits, generics, methods, and many more powerful features.

```rs
import core.stdc.stdio

fn main()
{
    printf("hello world!\n")
}
```

## Table of contents

- [Pars](#pars)
  - [Table of contents](#table-of-contents)
  - [Functions](#functions)
  - [Variables](#variables)
      - [var](#var)
      - [let](#let)
      - [const](#const)
  - [Aliases](#aliases)
  - [Pointers](#pointers)
  - [Arrays and Slices](#arrays-and-slices)
      - [Named elements](#named-elements)
      - [Array operations](#array-operations)
      - [slices](#slices)
      - [array properties](#array-properties)
  - [Control flow](#control-flow)
      - [if](#if)
      - [logical operators](#logical-operators)
      - [loops](#loops)
  - [Structs](#structs)
  - [Modules](#modules)
      - [Multiplatform imports](#multiplatform-imports)
      - [private symbols](#private-symbols)
  - [Builtin types](#builtin-types)
      - [Integer types](#integer-types)
      - [other primitives](#other-primitives)
  - [Type casting](#type-casting)
  - [Building pars](#building-pars)
  - [Technical details](#technical-details)

## Functions

a parse function is declared using the `fn` keyword.

```rs
fn say_hello()
{
    printf("hello")
}
```

the same function can be written in a single line with the arrow syntax

```rs
fn say_hello() => printf("hello")
```

a function such as this will always be inlined.

a function can have zero or more parameters and a return type

```rs
fn add(a: i32, b: i32): i32
{
    return a + b
}

// or

// return type is inferred
fn add(a: i32, b: i32) => a + b
```

functions can also have default parameters 

```rs
fn panic(message: str, code: i32 = -1)
{
    printf("%s", message)
    exit(code)
}
```

function parameters can also be named

```rs
panic(message: "somethings wrong", code: 32)
```

named and positional arguments can be used together as long as named arguments are used last and the order of named arguments can be any order.

## Variables
pars has three types of variables: var, let, const.

#### var
a mutable variable is defined with the var keyword.

```cs
var x = 10
```

in the above case the type is inferred but pars is a very strongly typed language.

```cs
var x: i32 = 10
```

in this case the type of x is explicitly used.

```cs
var x: i32
```
the initializer is optional but both either an initializer or an explicit type needs to be annotated.

#### let
let makes the variable itself become immutable at the top level.

```rs
let x = 10
```
x cannot be reassigned now.

```rs
let numbers = [1, 2, 3]

numbers[0] = 10
```

the above is perfectly valid pars code even through we used let. because let is essentially short hand for the following code

```cs
var numbers: imut [3]i32
```

the immutability is defined at the top level.

in order to make the elements immutable one can do:

```rs
let numbers: [?]imut i32 = [1, 2, 3]
```

this gives the user a great deal of control of what is and isnt immutable. this positional immutability can be applied anywhere in the type tree.

#### const
a const variable is a compile time variable that only exists in the compilers memory.

```go
const SIZE = 1024

var buffer: [SIZE]u8
```

the above shows a common use case for const variables as it is somewhat like C defines but unlike C defines it is a regular part of the pars frontend.

you cannot take the address of consts.

## Aliases

an alias is a way to give a new name to an existing type or create a unique version of it.

```cs
alias Integer = i32
```

now Integer will be a part of the type system but it is identical to an i32.

in order to make Integer be treated as its own unique type we must use the distinct keyword.

```cs
alias Integer = distinct i32
```

## Pointers

a pointer in pars uses an unconventional syntax to avoid ambiguity.

```go
var x = 10

// take the address of x
// ^ denotes a pointer. the compiler can infer the type but we explicitly type it here for demonstration.
var ptr: ^i32 = &x

// we can dereference the pointer with ^ and either read or write to it.
^ptr = 200
```

pars allows for unrestricted pointer arithmetic and it is the job of the programmer to use it carefully.

you can use the nil as a zero pointer literal just like null in other languages. 

```go
// the type of nil is '^void'
var ptr = nil
```

in pars any pointer can be assigned to a void pointer and it essentially acts as a any pointer. but void pointers cannot be dereferenced.

```rs
fn take_ptr(ptr: ^void) 
{
    
}

fn main()
{
    var ptr: ^i32

    take_ptr(ptr)
}
```

this allows for great c interop

## Arrays and Slices

an array literal is enclosed within square brackets

```cs
var numbers = [1, 2, 3]
```

the type of the array is inferred from its first element.

but it can also be specified like so:

```cs
var numbers = i32[1, 2, 3]
```

an array can be manually typed like so:

```go
var numbers: [3]i32
```

you can allow the compiler to infer its size from the right hand side by using a question mark

```go
var numbers: [?]i32 = [1, 2, 3]
```

but it must have an initializer otherwise the compiler will throw an error.

an array can be accessed by using the subscript operator

```go
var numbers = [1, 2, 3]

printf("%d\n", numbers[0])
```

#### Named elements
an array can have named elements.

```go

// {x, y} will now be the names of index 0 and 1
alias Vec2 = [2]i32{x, y}

var position: Vec2

// can still use the subscript operator but this makes the code more readable
position.x = 10
position.y = -5
```

this can be very useful for representing vectors and other math types.

the names can also be used during initialization.

```go
var position = Vec2[x: 10, y: -5]
```

#### Array operations

another useful math feature is array programming. all fixed sized arrays can perform the same math operations as normal integers such as addition or multiplication

```go
var a = [1, 2]
var b = [10, 20]

var result = a + b
```

this code will very likely compile down to simd instructions and thus will be very fast to perform.

arrays can also be nested. heres an example of how to do matrices using nested arrays

```go

    // can be explicitly typed like this
    // var mat: [4][4]i32

    var mat =
    [
        [1, 22, 3, 47],
        [51, 60, 7, 8],
        [32, 5, 12, 8],
        [0, 9, 12, 31]
    ]

    for n in mat
    {
        for k, i in n
        {
            printf("%d", k)

            if i < n.length
            {
                printf(",")
            }
        }

        printf("\n")
    }
```

#### slices

A slice is like an array but its size is not known at compile time. a slice is essentially just a pointer to some contiguous memory and a length.

```go
var numbers = [1, 2, 3, 4, 5]

// portion will be [2, 3]
var portion = numbers[1:2]
```

a slice does not own its memory it just has a view to some other memory. it is great as a lightweight view of memory and should often be the preferred form of accepting arrays.

a slice is explicitly typed with empty square brackets:

```go
var my_slice: []i32
```

a commonly used version of slices is the builtin string type `str` which is just defined as a `alias str = []imut char`

#### array properties

both slices and arrays have builtin properties.

```go
var numbers = [1, 2, 3]

printf("ptr = %p, length = %d\n", numbers.ptr, numbers.length)
```

a slice has the same properties. the ptr property will just return a pointer to the array or slices underlying memory and length is self explanatory.

## Control flow

pars has many forms of control flow

#### if

an if statement in pars is composed of a condition and a body and an else block that can recursively repeat.

```go 

if condition 
{
    do_stuff()
}
else 
{
    do_other_stuff()
}
```

#### logical operators

pars uses 'and' and 'or' for its logical operators for better readability.

```py
if condition and condition2
{
    do_stuff()
}

if condition or condition2
{
    do_stuff()
}
```

if statements can be done at compile time 

```rs
$if condition 
{

}
```
a dollar sign denotes a compile time if statement and its condition must be known at compile time. this is the way you can do conditional code generation in pars and can be useful for platform specific code.

#### loops
parse has 3 forms of loops

the first being the range based for loop
```rs

for n in 0..10
{
    printf("#%d\n", n)
}
```

a range based for loop can iterate on any range. the most basic being a range expression which works in a form of `low..high` and it is exclusive but an inclusive form can be used like so `low..=high`

arrays are also ranges

```rs
for n in my_array
{
    printf("#%d\n", n)
}
```

in the above case it will iterate through and print ever element of `my_array`.

a second binding can be used for the current index

```rs
for n, i in my_array
{
    printf("#%d (%d)\n", n, i)
}
```

now i will be the index at every iteration.

the second form of loop in pars is the while loop

```rs
while condition
{
    printf("condition is still true\n")
}
```

and finally the plain old loop

```rs
loop 
{
    printf("running forever\n")
}
```

this type of loop compiles down to a simple while true loop and can be useful for application loops.

during all loops `break` keyword can be used to break out of the current loop and `continue` to skip the rest of this iteration.

## Structs

a struct can be defined with the struct keyword

```rs
struct User
{
    name: str
    score: i32
    is_valid: bool
}
```

optionally each field can be separated with a comma if you want more readability when defining the struct inline such as `struct Pair {key: str, value: i32}`

a struct can be initialized in all the following ways

```go
var user: User = {score: 50, name: "henry"}
var user2 = User{"henry", score: 50}
var user3 = User{name: "henry", score: 50}
var user4 = User{"henry", score: 50, true}
var user5 = User{score: 50, true}
var user6 = User{score: 50}
```

anywhere where the type of the struct is known such as in a function argument or an explicitly typed variable one can just use the braces and leave out the struct name when initializing.

a struct member can be accessed like so

```go
var user = User{name: "henry", score: 50}

user.score += 100
```

structs can always be used anonymously

```rs
struct User
{
    name: str
    metadata: 
    {
        header: str
        value: u32
    }
}
```
this is useful for writing structs that are meant to be used for json or some other serialization format.


## Modules

pars has a very simple and logical module system.

a folder is a package and each file in that folder is a module.

imagine you have a root folder that has a subfolder called foo and within foo you have a file called greeter.pars

/foo/greeter.pars
```py
import core.stdc.stdio
    
fn greet() => printf("hello!\n")
```

inside the main file one can import it like so

/main.pars
```py
import foo.greeter

fn main()
{
    greet()
}
```

to import the package itself a package.pars file must exist within the package then when doing `import foo` the package.pars file will be imported

packages can also have sub packages.

there are multiple different ways to import such as

named imports make it so that all symbols imported must use a specific name to be accessed. this helps reduce name collisions.
```py
import fo_gr = foo.greeter 

fn main()
{
    fo_gr.greet()
}
```

selective imports allow importing only specific symbols. in this case only greet will be imported.
```py
import foo.greeter : greet

fn main()
{
    greet()
}
```

selective imports can also be aliased. now greet can only be used as the name gr

```py
import foo.greeter : gr = greet

fn main()
{
    gr()
}
```

a module can be imported at any scope and its imported symbols will only be accessible within that scope

```py
fn main()
{
    import foo.greeter

    greet()
}
```

in the above code greet can only be called inside of main.

this is useful for avoiding naming collisions.

#### Multiplatform imports

for writing multiplatform code it is possible to suffix a file with the format of `mod_name.platform.pars` this will make it so that you can have multiple different versions of each module for each platform such as `virtual_memory.linux.pars` for linux and `virtual_memory.windows.pars` for windows and the compiler will pick the correct one to compile and import depending on the compilation target.

#### private symbols

a symbol can become private to stay internal to a module by using the private attribute

```d
@private
fn secret_function()
{

}
```

now secret_function cannot be exported from this module.

## Builtin types

all builtin types have the default value of 0 or nil in case of pointers. to get the init value of any type one can use the init type property:

```rs
printf("%d\n", i32.init)
```

to get the size of any type use the sizeof expression

```c
printf("%d\n", sizeof(i32))
```

#### Integer types

parses integer types use a very logical format. they are prefixed with either i for signed and u for unsigned followed by a bit size such as `i32` for a signed 32 bit integer. in total these are the supported integers

- i8
- u8
- i16
- u16
- i32
- u32
- i64
- u64

#### other primitives

- `char`: can be used for single characters.
- `bool`: is used for true and false expressions.
- `void`: used to denote a lack of a type

## Type casting

the `cast` expression can be used to change the type of an expression like so:

```go
var c: char = cast(char)10
```

any expression can be used withing the brackets for cast and the type of the result of that expression will be used.


## Building pars

in order to build pars several things are required:

- lib llvm 20 
- cmake
- linux (windows support will come in the future)

when all dependencies are met simply invoke cmake to build the project.

## Technical details

pars is written in a very idiomatic version of C++. At the beginning i utilized std::variant and std::visit and overload pattern to implement the frontend but i found C++ to be very awkward with this approach. i then resorted to using inheritance to model the frontend nodes and this approach did end up being far more natural and smooth but i would prefer the first approach if the language had good support for it. in many places i also use exceptions despite the fact that i absolutely hate exceptions. this is because it is the most natural way to do unrecoverable errors that must propagate up in C++ and thus it is used entirely for errors that will stop compilation.

pars takes a fairly standard approach to its memory management that most compilers use and that it does not manage it at all. everything is allocated in several large arenas that are never freed. this results in extremely fast compile times. in fact in my profiling only 0.34% of compile times is spent in the ast and 2% compiling an entire module.

There are likely more than a few places where you might see uncommon algorithms and solutions to typical problems as i reinvented the wheel in most places so when reading the source code one might be puzzled if it was written by a chimp or not.
