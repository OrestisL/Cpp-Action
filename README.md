# C++ Action

A lightweight, header-only C++ implementation of C#'s `Action` delegate/event pattern. Subscribe any number of callbacks to an `Action`, then invoke them all with a single call.

## Features

- **Header-only**: just drop `Action.hpp` into your project
- **C#-style API**: `AddListener`, `RemoveListener`, `Invoke`
- **Variadic template arguments**: `Action<int, int>`, `Action<std::string>`, `Action<>`, and so on
- **Works with free functions and lambdas**, including removing them later (see [Lambdas](#lambdas))
- **Thread-safe**: listener list access is protected by a mutex
- **Safe re-entrancy**: listeners may add or remove listeners on the same action while it is being invoked
- **Standard C++ only**: built on `std::function`, `std::shared_ptr`, `std::vector` and `std::mutex`

## Requirements

- C++14 or newer (the example project builds with C++20)
- Any standard-compliant compiler (GCC, Clang, MSVC)

## Usage

Copy `Action.hpp` into your project and include it:

```cpp
#include <iostream>
#include "Action.hpp"

void Add(int x, int y)
{
    std::cout << "Sum: " << x + y << std::endl;
}

void Subtract(int x, int y)
{
    std::cout << "Difference: " << x - y << std::endl;
}

int main()
{
    // Declare an action that takes two ints and returns void
    Action<int, int> someEvent;

    // Subscribe listeners
    someEvent.AddListener(Add);
    someEvent.AddListener(Subtract);

    // Invoke all listeners
    someEvent.Invoke(1, 2);

    // Unsubscribe a specific listener
    someEvent.RemoveListener(Add);

    // Unsubscribe everyone
    someEvent.RemoveAllListeners();
}
```

### Lambdas

Lambdas can be subscribed directly, but to be able to **remove** one later you need to keep it in a variable, just like keeping a delegate in a variable in C#. `AddListener` returns the listener it stored, so just keep the result and use it to unsubscribe:

```cpp
auto multiply = someEvent.AddListener([](int x, int y) {
    std::cout << "Product: " << x * y << std::endl;
});

someEvent.Invoke(3, 4);
someEvent.RemoveListener(multiply);
```

The return value can be ignored when you don't need it (for example with free functions). Inline lambdas whose result you discard can never be removed individually (same as in C#):

```cpp
someEvent.AddListener([](int, int) { std::cout << "can't be removed\n"; });
```

You can also build the listener first and add it later, or add one listener to several actions that share the same argument types. Declare it with the `Listener` type (use `auto` only on the result of `AddListener`, not on a bare lambda):

```cpp
Action<int, int>::Listener multiply = [](int x, int y) { /* ... */ };
actionA.AddListener(multiply);
actionB.AddListener(multiply);
```

### `+=` and `-=` operators

The C# operators are supported as well:

```cpp
someEvent += Add;        // same as AddListener(Add)
someEvent -= Add;        // same as RemoveListener(Add)
```

**Reusable lambdas:** `+=` returns the action (so it can be chained), which means it cannot return the `Listener` the way `AddListener` does. If you want to remove a lambda later, you can't use `auto x = (someEvent += lambda)`. Either use `AddListener` and keep its return value, or declare a `Listener` variable first and use it with both operators:

```cpp
Action<int, int>::Listener divide = [](int x, int y) {
    std::cout << x / y << std::endl;
};
someEvent += divide;
someEvent -= divide;
```

An inline lambda (`someEvent += [](int x, int y) { ... };`) subscribes fine but can never be removed.

### Invocation list

`GetInvocationList()` returns a snapshot (a copy) of the current listeners, like in C#. It is safe to use while the action is modified or invoked elsewhere. Each `Listener` can be called directly and compared with `==`. This is useful for counting subscribers, or for invoking listeners yourself, for example to catch exceptions per listener so one failing listener doesn't stop the others:

```cpp
std::cout << someEvent.GetInvocationList().size() << " listeners\n";

for (const auto& listener : someEvent.GetInvocationList()) {
    try {
        listener(1, 2);
    } catch (const std::exception& e) {
        std::cout << "listener threw: " << e.what() << '\n';
    }
}
```

Unlike C#, there is no way to inspect which function or object a listener points to (C++ has no reflection).

### C# comparison

| C# | C++ (this library) |
|---|---|
| `Action<int, int> e;` | `Action<int, int> e;` |
| `Action e;` | `Action<> e;` |
| `e += Add;` | `e += Add;` (or `e.AddListener(Add);`) |
| `e -= Add;` | `e -= Add;` (or `e.RemoveListener(Add);`) |
| `e?.Invoke(1, 2);` | `e.Invoke(1, 2);` |

The only syntax difference is that an action with no arguments is written `Action<>` in C++.

## API

| Method | Description |
|---|---|
| `AddListener(listener)` | Subscribes a callback (function, lambda, functor or `Listener`) and returns the stored `Listener` |
| `RemoveListener(listener)` | Unsubscribes the first matching listener |
| `operator+=` / `operator-=` | Same as `AddListener` / `RemoveListener`, but return the action, so they can't return a lambda's `Listener` |
| `GetInvocationList()` | Returns a snapshot of the current listeners as a `std::vector<Listener>` |
| `RemoveAllListeners()` | Clears all callbacks |
| `Invoke(args...)` | Calls every listener in the order added |

## Building the example

```bash
cmake -B build
cmake --build build
./build/csharp-events
```

## Limitations

- To remove a lambda, functor or `std::bind` result, keep the `Listener` returned by `AddListener` (or store it in an `Action<...>::Listener` variable and add that) and use it with `RemoveListener`. Passing a fresh inline lambda to `RemoveListener` removes nothing.
- `+=` cannot return the `Listener`, so removing a lambda with `-=` requires a `Listener` variable declared beforehand (see above).
- Free functions are matched by function pointer, so `RemoveListener(Add)` works as long as the function's signature matches the action's exactly. A function with merely convertible parameters (for example `void f(long, long)` on an `Action<int, int>`) is wrapped as a callable and must be removed through a stored listener instead.
- If a listener throws during `Invoke`, the exception propagates to the caller and the remaining listeners are not called (same as C#). Use `GetInvocationList()` to invoke listeners individually if you need per-listener error handling.
- `Invoke` works on a snapshot of the listeners. If a listener removes another one during an invoke, the removed one may still be called once in that pass.

## License

Released under the [MIT License](LICENSE).