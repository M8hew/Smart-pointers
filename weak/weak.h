#pragma once

#include "sw_fwd.h"  // Forward declaration

#include "shared.h"
// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename T>
class WeakPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    WeakPtr() {
    }

    WeakPtr(const WeakPtr& other) : obj_(other.obj_), block_(other.block_) {
        if (other.block_) {
            block_->IncrementWeak();
        }
    }

    WeakPtr(WeakPtr&& other) {
        Swap(other);
    }

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    WeakPtr(const SharedPtr<T>& other) : obj_(other.obj_), block_(other.block_) {
        if (block_) {
            block_->IncrementWeak();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr& operator=(const WeakPtr& other) {
        if (this == &other) {
            return *this;
        }
        Reset();
        obj_ = other.obj_;
        block_ = other.block_;
        if (block_) {
            block_->IncrementWeak();
        }
        return *this;
    }

    WeakPtr& operator=(WeakPtr&& other) noexcept {
        Reset();
        Swap(other);
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~WeakPtr() {
        Reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        obj_ = nullptr;
        if (block_) {
            block_->DecrementWeak();
            if (block_->CanBlockBeDeleted()) {
                delete block_;
            }
            block_ = nullptr;
        }
    }

    void Swap(WeakPtr& other) {
        std::swap(obj_, other.obj_);
        std::swap(block_, other.block_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const {
        if (block_) {
            return block_->GetSharedCount();
        }
        return 0;
    }

    bool Expired() const {
        if (block_) {
            return block_->GetSharedCount() == 0;
        }
        return true;
    }

    SharedPtr<T> Lock() const {
        return SharedPtr<T>(*this, true);
    }

private:
    T* obj_ = nullptr;
    BaseControlBlock* block_ = nullptr;
    friend class SharedPtr<T>;
};
