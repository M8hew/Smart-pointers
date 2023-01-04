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

    // May be used for ESFT
    WeakPtr(const WeakPtr& other) : obj_(other.obj_), block_(other.block_) {
        if (other.block_) {
            block_->IncrementWeak();
        }
    }

    // May be used for ESFT
    template <typename Y>
    WeakPtr(const WeakPtr<Y>& other) : obj_(static_cast<T*>(other.obj_)), block_(other.block_) {
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

    // For ESFT
    WeakPtr(T* obj, BaseControlBlock* block)
        : obj_(obj), block_(block), can_i_delete_block_(false) {
        if (block_) {
            block_->IncrementWeak();
        }
    }

    // For ESFT
    template <typename Y>
    WeakPtr(Y* obj, BaseControlBlock* block)
        : obj_(static_cast<T*>(obj)), block_(block), can_i_delete_block_(false) {
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
        can_i_delete_block_ = other.can_i_delete_block_;
        if (block_) {
            block_->IncrementWeak();
        }
        return *this;
    }

    template <typename Y>
    friend class WeakPtr;

    template <typename Y>
    WeakPtr<T>& operator=(const WeakPtr<Y>& other) {
        Reset();
        obj_ = other.obj_;
        block_ = other.block_;
        can_i_delete_block_ = other.can_i_delete_block_;
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
            if (block_->CanBlockBeDeleted() && can_i_delete_block_) {
                delete block_;
            }
            block_ = nullptr;
        }
    }

    void Swap(WeakPtr& other) {
        std::swap(obj_, other.obj_);
        std::swap(block_, other.block_);
        std::swap(can_i_delete_block_, other.can_i_delete_block_);
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
    bool can_i_delete_block_ = true;
    friend class SharedPtr<T>;
};
