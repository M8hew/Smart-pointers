#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t

class BaseControlBlock {
public:
    virtual void IncrementShared() {
        ++shared_counter_;
    }

    virtual void DecrementShared() {
        --shared_counter_;
        if (shared_counter_ == 0) {
            DeleteData();
        }
    }

    virtual size_t GetSharedCount() {
        return shared_counter_;
    }

    virtual void IncrementWeak() {
        ++weak_counter_;
    }

    virtual void DecrementWeak() {
        --weak_counter_;
    }

    virtual size_t GetWeakCount() {
        return weak_counter_;
    }

    virtual bool CanBlockBeDeleted() {
        return shared_counter_ == 0 && weak_counter_ == 0;
    }

    virtual void DeleteData() = 0;

    virtual ~BaseControlBlock() = default;

protected:
    size_t shared_counter_ = 0;
    size_t weak_counter_ = 0;
};

template <typename T>
class PtrControlBlock : public BaseControlBlock {
public:
    PtrControlBlock() : ptr_(nullptr) {
    }

    PtrControlBlock(T* ptr) : ptr_(ptr) {
        this->IncrementShared();
    }

    void DeleteData() override {
        if (ptr_) {
            delete ptr_;
            ptr_ = nullptr;
        }
    };

    ~PtrControlBlock() {
        this->DeleteData();
    }

private:
    T* ptr_;
};

template <typename T>
class MakeSharedControlBlock : public BaseControlBlock {
public:
    MakeSharedControlBlock() {
        this->IncrementShared();
    }

    void DeleteData() override {
        if (!was_data_deleted_) {
            Get()->~T();
            was_data_deleted_ = true;
        }
    }

    ~MakeSharedControlBlock() {
        if (!was_data_deleted_) {
            Get()->~T();
        }
    }

    auto GetStorageRef() {
        return &obj_;
    }

    T* Get() {
        return reinterpret_cast<T*>(&obj_);
    }

private:
    typename std::aligned_storage<sizeof(T), alignof(T)>::type obj_;
    bool was_data_deleted_ = false;
};

// https://en.cppreference.com/w/cpp/memory/shared_ptr
template <typename T>
class SharedPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr() {
    }

    SharedPtr(std::nullptr_t) {
    }

    explicit SharedPtr(T* ptr) : obj_(ptr), block_(new PtrControlBlock(ptr)) {
    }

    SharedPtr(const SharedPtr& other) : obj_(other.obj_), block_(other.block_) {
        if (block_) {
            block_->IncrementShared();
        }
    }

    SharedPtr(SharedPtr&& other) noexcept {
        Swap(other);
    }

    SharedPtr(T* obj, BaseControlBlock* block) : obj_(obj), block_(block) {
    }

    template <typename Y>
    friend class SharedPtr;

    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other) : obj_(other.obj_), block_(other.block_) {
        if (block_) {
            block_->IncrementShared();
        }
    }

    template <typename Y>
    SharedPtr(SharedPtr<Y>&& other) noexcept : obj_(other.obj_), block_(other.block_) {
        other.obj_ = nullptr;
        other.block_ = nullptr;
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) : obj_(ptr), block_(other.block_) {
        if (block_) {
            block_->IncrementShared();
        }
    }

    // control block of Y, observing T*
    template <typename Y>
    SharedPtr(Y* ptr) : obj_(ptr), block_(new PtrControlBlock<Y>(ptr)) {
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other, bool from_lock = false)
        : obj_(other.obj_), block_(other.block_) {
        if (block_) {
            if (other.Expired()) {
                if (from_lock) {
                    obj_ = nullptr;
                } else {
                    throw BadWeakPtr();
                }
            }
            block_->IncrementShared();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (this == &other) {
            return *this;
        }
        Reset();
        obj_ = other.obj_;
        block_ = other.block_;
        if (block_) {
            block_->IncrementShared();
        }
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        Reset();
        Swap(other);
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        Reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        obj_ = nullptr;
        if (block_) {
            block_->DecrementShared();
            if (block_->CanBlockBeDeleted()) {
                delete block_;
            }
            block_ = nullptr;
        }
    }

    void Reset(T* ptr) {
        Reset();
        obj_ = ptr;
        block_ = new PtrControlBlock<T>(ptr);
    }

    template <typename Y>
    void Reset(Y* ptr) {
        Reset();
        obj_ = ptr;
        block_ = new PtrControlBlock<Y>(ptr);
    }

    void Swap(SharedPtr& other) {
        std::swap(obj_, other.obj_);
        std::swap(block_, other.block_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return obj_;
    }

    T& operator*() const {
        return *Get();
    }

    T* operator->() const {
        return Get();
    }

    size_t UseCount() const {
        if (block_) {
            return block_->GetSharedCount();
        }
        return 0;
    }

    explicit operator bool() const {
        return block_ != nullptr;
    }

private:
    T* obj_ = nullptr;
    BaseControlBlock* block_ = nullptr;
    friend class WeakPtr<T>;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right);

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    MakeSharedControlBlock<T>* block_ptr = new MakeSharedControlBlock<T>;
    try {
        new (block_ptr->GetStorageRef()) T(std::forward<Args>(args)...);
    } catch (...) {
        delete block_ptr;
        throw;
    }
    return SharedPtr<T>(block_ptr->Get(), block_ptr);
}

// Look for usage examples in tests
template <typename T>
class EnableSharedFromThis {
public:
    SharedPtr<T> SharedFromThis();
    SharedPtr<const T> SharedFromThis() const;

    WeakPtr<T> WeakFromThis() noexcept;
    WeakPtr<const T> WeakFromThis() const noexcept;
};
