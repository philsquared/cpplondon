# How to improve exception throwing

By Sumant Hanumante and Eduardo Madrid [youtube](https://www.youtube.com/watch?v=euMWIcTrwQQ)

* Template code bloat judo
* The best types are the most representative
* Better than “perfect forwarding”

## Abstract

C++ templates are notorious for causing object code bloat, in this presentation we show how, paradoxically, we can use templates to **reduce object code size**.

The object code the compiler inlines to throw exceptions may happen interleaved with critical code, if we factor it out we can not just benefit from reducing the total object code size but also performance due to **improving the efficiency of the instruction cache**.

Exception throwing turns into some boilerplate object code that we can reduce by

1. Careful choices on what to inline, what not to inline,
2. Forwarding trivial types by value and
3. Minimizing template instantiations by representing a family of types such as arrays of characters, string views, using a common type such plain character pointers.

We suspect the combination of these techniques is original.

## What are the improvements?

Reduction of object code size, possibly also a little bit better performance:

We use these techniques for the library that does Augmented Reality within Snapchat, and we save about 1.5% object code size.

Forwarding trivial types by value actually reduces the amount of work the processor needs to do, that is a small performance improvement when applicable, also the object-code-size inefficiency of “naked” throws may happen very close to performance critical code and it may reduce the efficiency of the instruction cache.

These techniques are useful independently, for example, some are used in the “Zoo” libraries for better performance.

## Who are the presenters, what’s this code’s pedigree?

Sumant Hanumante is the main author of these improvements, we both work at the group that does Augmented Reality at Snap, Inc., the maker of Snapchat, doing mostly performance oriented C++.

This is the first broadly available presentation by Sumant.

You may recognize Eduardo from his presentations at CPPCon.

These techniques owe work done from our coworker Denys Makoviichuk and shown in our team’s poster (Evgenii Zaikin, Fedir Poliakov, Yurii Monastyrshin) that won the CPPCon 2019 competition; and also Facebook’s `folly` and Eduardo’s `zoo` libraries.  We think the combination of all four techniques is original.

Even better, this is running in production and used all over the world.

Aspects of this subject matter are covered in more depth in two presentation proposals for this year’s CPPCon, if not accepted, Eduardo will continue to try to find a venue to share their content.

## Normal exception throwing

To throw an exception, the runtime needs to do these things:

1. Allocate space for an exception object in a special section of memory
2. Construct the exception object there
3. Bring in the `type_info` of the exception type to allow `catch` statements to filter, the destructor of the exception object for it to be destroyed after the `catch`.
4. Activate the stack unwinding

Unfortunately, the compilers inline all of these things each `throw` statement, see:
Example 1:
https://godbolt.org/z/xK4yHD
The exception throwing object code is repeated, even if what it does is essentially the same thing.

We can factor most of this into a function whose object code is reused.

## How to improve?

### Step 1: Factor exception throwing into a trampoline function

By taking care of exception throwing into a function we have a single place to improve exception throwing:

```
template<class Ex, class... Args>
[[noreturn]] ATTRIBUTE_ALWAYS_INLINE
auto throwException(Args &&...args) ->
    std::enable_if_t<
        std::is_base_of_v<std::exception, Ex>
    >;
```

This needs to be:

1. A template that takes an exception type as the first template parameter
2. Uses forwarding references to not change the value category of the arguments
3. Should have the attribute “noreturn”
4. Should be inlined, give a stronger hint to the compiler than just `inline`.
5. Optionally, for introspection, make sure the exception type inherits from `std::exception`, in this example, using the technique of “SFINAE” on the return type (the syntax is “trailing return type”).

### Step 2: Split exception throwing into an inlined frontend and non-inlined backend:

The first level of object code reuse we can get is to pay only once the object code for a particular exception type and construction parameters.  In example 1 we throw `MyError` built from `const char *` in two places.  We can encapsulate this into a non-inlined function:

```
template<typename Ex, typename... Args>
[[noreturn]] __attribute__((noinline))
void backend(Args &&...args) {
    throw Ex(std::forward<Args>(args)...);
}

template<typename Ex, typename... Args>
[[noreturn]] __attribute__((always_inline))
void throwException(Args &&...args) {
    backend<Ex>(std::forward<Args>(args)...);
}
```

We already have benefits!
https://godbolt.org/z/LUVqXT

But we cheated: the messages “boolean failed” and “pointer failed” happen to be of the exact same size, if we use different sizes, then we instantiate the backend twice, **because we are passing two different types of arguments, a string literal is a `const char (&)[Length])`, different lengths are different types!**

**This combination of inline/non-inline allows the compiler to optimize as much as possible in both sides without having to pay the object code penalty of repetition.**

### Step 3: “Project” families of types into a single representative or “the springs in the internal trampoline”

We do not really need to instantiate the backend for each individual type we are passing, we can instead project different types to a “representative”.  For example, for a string argument, we can convert or “project” the argument to `const char *`, the same with `string_view`, character buffers.  Also, the same thing for arithmetic types:

`const char (&)[L]` → `const char *`
`std::string_view` → `const char *`
`std::string` → `const char *`
`int` → `long`
`short` → `long`
`float` → `double`

We are changing a little bit the meaning of the code, but that is something that happens often when we are doing any kind of improvement:  For something to be better necessarily it first needs to be different.  It is our judgement as software engineers what allows us to find opportunities for improvement.

We will explain the code in this example
https://godbolt.org/z/Q4DWHz

1. Using overload resolution as a pattern matching engine to avoid the gratuitous repetition of flavors of the same type
2. Converting overload resolution into a type trait
3. Pattern-matching on a type trait
4. Making projection sort of “springs” in a trampoline

### Step 4: better than “perfect forwarding”

“Perfect forwarding” refers to preserving the value category of an argument of the current function when giving it as argument to another function.

One nagging issue with “perfect forwarding” is that it turns trivial types (such as integers) into r-value references; that is, things that don’t need to have an address are rather copied to the stack and their address in the stack is the actual thing passed.  That is wasteful.

We can trampoline the making of the exception argument itself, to pass by value if the argument is not a modifiable (non - `const`) l-value reference and the base type is trivial.
https://godbolt.org/z/DV2Wzo

### Step 5 and beyond

These techniques to reduce template instantiations can be potentiated by using introspection of constructibility of the exception object.  Take the example of exception types whose constructors are `std::string` or `std::string &&`, the intention seems clear: the exception object is building a string out of pieces that are given, in this case, projecting from `std::string` to `const char *` so that the constructor converts again to `std::string` is undesirable.  This case can be detected using the `is_constructible` type trait and making the necessary adjustments in the trampolining.

Another consideration is how to allow the user to express their own ways to project arguments.

### Other choices

We have seen Facebook’s `folly` mechanism for throwing exceptions
https://github.com/facebook/folly/blob/792d3247e17c69c1189b2f1594d777b4eb76e1d9/folly/lang/Exception.h#L36

We measured their technique is detrimental compared to the combination of techniques we use.  Why?

Perhaps because factoring out the creation of the exception object does not pay for itself because it then requires a moving to the memory only `throw` has access to, and also destroying the source, the temporary exception object, which may involve typically invoking an overriden destructor.

## More general application

These techniques are more broadly applicable:

1. We described ways to reduce function template instantiations
2. We described ways to forward by value trivial types as opposed to by stack-reference

Both are applicable to runtime polymorphism:

A library that has a class hierarchy might be supported by facilities that use templates, we know that the parameters to `virtual` overrides are covariant, this will make the specific argument types in the derived classes to generate code bloat.


Sumant does not maintain an online presence.

Eduardo's online coordinates:
Blog: https://github.com/thecppzoo/thecppzoo.github.io formerly https://thecppzoo.blogspot.com/
Github: https://github.com/thecppzoo
Twitter: [@thecppzoo](https://twitter.com/thecppzoo)
Slack cpplang: @thecppzoo
