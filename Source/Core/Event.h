#pragma once
// Tiny synchronous event bus. Gameplay emits; systems subscribe.
// Synchronous on the game thread keeps the simulation deterministic (§76).
#include <functional>
#include <vector>

namespace ink {

template <typename T>
class Event {
public:
    using Sink = std::function<void(const T&)>;

    void Listen(Sink s) { sinks_.push_back(std::move(s)); }
    void Emit(const T& value) const {
        for (const auto& s : sinks_)
            s(value);
    }
    void Clear() { sinks_.clear(); }
    std::size_t Count() const { return sinks_.size(); }

private:
    std::vector<Sink> sinks_;
};

} // namespace ink
