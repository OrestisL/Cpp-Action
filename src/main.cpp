#include "Action.hpp"
#include <exception>
#include <iostream>
#include <ostream>

void Add(int x, int y) {
  std::cout << "The result of adding is: " << x + y << std::endl;
}

void Subtract(int x, int y) {
  std::cout << "The result of subtracting is: " << x - y << std::endl;
}

int main() {
  std::cout << "Hello!\nThis is a random sample for using actions!\n";
  std::cout << "The syntax is just like C# actions: Action<T1,T2,....TN> to "
               "declare the action (returns void)\n";
  std::cout << "Use .AddListener / .RemoveListener to add / remove listeners "
               "like in C#.\n";
  std::cout << "Only syntax difference is that an Action with no arguments "
               "would be Action<> here, instead of just Action in C#.\n";
  std::cout << "The example of this file uses a simple Action with 2 ints as "
               "arguments.\n";

  // create action
  Events::Action<int, int> someTestEvent;

  // add listeners
  someTestEvent.AddListener(Add);
  someTestEvent.AddListener(Subtract);

  // invoke with the values you want, for example 1 and 2 here
  someTestEvent.Invoke(1, 2);

  // remove specific listener
  someTestEvent.RemoveListener(Add);

  // lambdas work too, but to be able to remove one later you must keep it
  // in a variable and use that same variable for add and remove
  // (just like keeping a delegate in a variable in C#).
  // AddListener returns the listener it stored, so just keep the result.
  auto multiply = someTestEvent.AddListener([](int x, int y) {
    std::cout << "The result of multiplying is: " << x * y << std::endl;
  });

  std::cout << "\nSubtract + multiply lambda:\n";
  someTestEvent.Invoke(3, 4);

  // remove the lambda using the same variable
  someTestEvent.RemoveListener(multiply);

  std::cout << "\nOnly subtract is left:\n";
  someTestEvent.Invoke(3, 4);

  // note: an inline lambda like this can never be removed, same as in C#
  someTestEvent.AddListener([](int, int) { std::cout << "inline lambda\n"; });

  // C#-style operators are also available: += subscribes, -= unsubscribes
  someTestEvent += Add;
  someTestEvent -= Add;

  // += cannot return the listener, so to remove a lambda with -= you must
  // declare a Listener variable first and use it with both operators
  Events::Action<int, int>::Listener divide = [](int x, int y) {
    std::cout << "The result of dividing is: " << x / y << std::endl;
  };
  someTestEvent += divide;
  someTestEvent -= divide;

  // GetInvocationList returns a snapshot of the current listeners.
  // Useful for counting them, or for invoking each one yourself, e.g. to
  // catch exceptions per listener so one failure doesn't stop the rest.
  someTestEvent.RemoveAllListeners();
  someTestEvent += Add;
  someTestEvent += Subtract;
  std::cout << "\nListener count: " << someTestEvent.GetInvocationList().size()
            << std::endl;
  for (const auto &listener : someTestEvent.GetInvocationList()) {
    try {
      listener(10, 5);
    } catch (const std::exception &e) {
      std::cout << "A listener threw: " << e.what() << std::endl;
    }
  }

  // clear event
  someTestEvent.RemoveAllListeners();
}