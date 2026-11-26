#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>

namespace Events{
  template <class... Args> class Action {
  public:
    // A listener is a cheap, copyable handle to a callable. All copies of the
    // same Listener share one underlying callable, which gives it an identity
    // (like a C# delegate): store a lambda in a Listener variable, add it,
    // and remove it later with that same variable.
    class Listener {
    public:
      using Signature = void(Args...);
      using FnPtr = Signature *;

      Listener() = default;

      // From a plain function (or function pointer). Identity is the pointer
      // itself, so wrapping the same function twice yields equal listeners.
      Listener(FnPtr fn)
          : m_fn(fn),
            m_callable(fn ? std::make_shared<std::function<Signature>>(fn)
                          : nullptr) {}

      // From any other callable (lambda, functor, std::bind result, ...).
      // Identity is the shared callable created here.
      template <class F,
                class = std::enable_if_t<
                    !std::is_same<std::decay_t<F>, Listener>::value &&
                    !std::is_same<std::decay_t<F>, FnPtr>::value &&
                    std::is_constructible<std::function<Signature>, F>::value>>
      Listener(F &&callable)
          : m_callable(std::make_shared<std::function<Signature>>(
                std::forward<F>(callable))) {}

      void operator()(const Args &...args) const {
        if (m_callable && *m_callable)
          (*m_callable)(args...);
      }

      friend bool operator==(const Listener &a, const Listener &b) {
        if (a.m_fn || b.m_fn)
          return a.m_fn == b.m_fn;
        return a.m_callable == b.m_callable;
      }

      friend bool operator!=(const Listener &a, const Listener &b) {
        return !(a == b);
      }

    private:
      FnPtr m_fn = nullptr;
      std::shared_ptr<std::function<Signature>> m_callable;
    };

    inline Action() {}

    // Subscribes a listener and returns it. The returned Listener shares
    // identity with the stored one, so keep it if you want to remove a lambda
    // later: `auto l = action.AddListener(...); action.RemoveListener(l);`
    // The return value can be ignored (e.g. for free functions).
    inline Listener AddListener(Listener listener) {
      std::lock_guard<std::mutex> guard(m_mtx);
      m_listeners.push_back(listener);
      return listener;
    }

    // Removes the first listener equal to the given one.
    // Works for free functions, and for lambdas/functors that were stored in a
    // Listener variable and added from that same variable. Passing a fresh
    // inline lambda is safe but removes nothing (same as C#).
    inline void RemoveListener(const Listener &listener) {
      std::lock_guard<std::mutex> guard(m_mtx);
      auto it = std::find(m_listeners.begin(), m_listeners.end(), listener);
      if (it != m_listeners.end())
        m_listeners.erase(it);
    }

    // C#-style operators. Passing a function or lambda converts implicitly to
    // a Listener. operator+= returns *this (by convention), so it cannot hand
    // back the Listener: to remove a lambda later with -=, store it in an
    // Action<...>::Listener variable first and use that with both operators.
    inline Action &operator+=(Listener listener) {
      AddListener(std::move(listener));
      return *this;
    }

    inline Action &operator-=(const Listener &listener) {
      RemoveListener(listener);
      return *this;
    }

    // Returns a snapshot (copy) of the current listeners, like C#'s
    // GetInvocationList. Each Listener is callable and comparable with ==.
    // Safe to use while the action is modified or invoked elsewhere.
    inline std::vector<Listener> GetInvocationList() {
      std::lock_guard<std::mutex> guard(m_mtx);
      return m_listeners;
    }

    inline void RemoveAllListeners() {
      std::lock_guard<std::mutex> guard(m_mtx);
      m_listeners.clear();
    }

    inline void Invoke(const Args &...args) {
      // Copy under the lock, invoke outside it. This lets listeners call
      // AddListener/RemoveListener on this action without deadlocking.
      std::vector<Listener> snapshot;
      {
        std::lock_guard<std::mutex> guard(m_mtx);
        if (m_listeners.empty())
          return;
        snapshot = m_listeners;
      }

      for (const auto &listener : snapshot)
        listener(args...);
    }

  private:
    std::vector<Listener> m_listeners;
    std::mutex m_mtx;
  };
} // namespace