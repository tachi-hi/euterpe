// guiBase.h
//
// Base class for GUI parameter providers.
// Shared by GUI (Tcl/Tk) and QtGUI versions.
// Uses std::atomic<T> instead of myMutex<T> for lock-free parameter access.

#pragma once
#include <atomic>

// Atomic parameter wrapper with get()/set() interface.
// Drop-in replacement for myMutex<T> for scalar parameters.
template<typename T>
class AtomicParam {
public:
    AtomicParam() noexcept : value_{T{}} {}
    AtomicParam(T init) noexcept : value_{init} {}
    T    get() const noexcept { return value_.load(std::memory_order_relaxed); }
    void set(T v)  noexcept { value_.store(v, std::memory_order_relaxed); }

private:
    std::atomic<T> value_;
};

// Base class providing shared parameters for the processing pipeline.
class GUIBase {
public:
    AtomicParam<int> key{0};
    AtomicParam<int> tempo{100};
    AtomicParam<int> volume{100};

    bool getstop() const noexcept { return stop_.load(std::memory_order_relaxed); }
    bool getquit() const noexcept { return quit_.load(std::memory_order_relaxed); }
    void setstop()       noexcept { stop_.store(true, std::memory_order_relaxed); }
    void setquit()       noexcept { quit_.store(true, std::memory_order_relaxed); }

    GUIBase() = default;
    virtual ~GUIBase() = default;

    // Non-copyable, non-movable (std::atomic members are not copyable)
    GUIBase(const GUIBase&)            = delete;
    GUIBase& operator=(const GUIBase&) = delete;
    GUIBase(GUIBase&&)                 = delete;
    GUIBase& operator=(GUIBase&&)      = delete;

protected:
    std::atomic<bool> stop_{false};
    std::atomic<bool> quit_{false};
};
