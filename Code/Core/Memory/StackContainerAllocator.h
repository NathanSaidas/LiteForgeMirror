// ********************************************************************
// Copyright (c) 2021 Nathan Hanlan
// 
// Permission is hereby granted, free of charge, to any person obtaining a 
// copy of this software and associated documentation files(the "Software"), 
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and / or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in 
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE 
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ********************************************************************
#pragma once

#include <vector>
#include "Core/Memory/AlignedMemory.h"

namespace lf {

template<typename T, size_t StackCapacityT>
class StackAllocator : public std::allocator<T> 
{
public:
    using pointer = typename std::allocator<T>::pointer;
    using size_type = typename std::allocator<T>::size_type;

    struct Buffer 
    {
        Buffer()
        : mMemory()
        , mUsed(false)
        {
        }

        T* Data() { return reinterpret_cast<T*>(mMemory.mData); }
        const T* Data() const { return reinterpret_cast<const T*>(mMemory.mData); }

        AlignedMemory<sizeof(T[StackCapacityT]), alignof(T)> mMemory;
        bool mUsed;
    };

    template<typename U>
    struct rebind 
    {
        using other = StackAllocator<U, StackCapacityT>;
    };

    StackAllocator(const StackAllocator<T, StackCapacityT>& rhs)
    : std::allocator<T>()
    , mBuffer(rhs.mBuffer) 
    {
    }
    
    template<typename U, size_t StackCapacityT>
    StackAllocator(const StackAllocator<U, StackCapacityT>& )
    : mBuffer(nullptr) 
    {
    }

    explicit StackAllocator(Buffer* buffer) 
    : mBuffer(buffer) 
    {
    }
    
    pointer allocate(size_type n, void* hint = 0) 
    {
        if (mBuffer != NULL && !mBuffer->mUsed && n <= StackCapacityT) 
        {
            mBuffer->mUsed = true;
            return mBuffer->Data();
        }
        else 
        {
            return std::allocator<T>::allocate(n, hint);
        }
    }
    
    void deallocate(pointer p, size_type n) 
    {
        if (mBuffer != NULL && p == mBuffer->Data())
        {
            mBuffer->mUsed = false;
        }
        else
        {
            std::allocator<T>::deallocate(p, n);
        }
    }
private:
    Buffer* mBuffer;
};

template<typename ContainerTypeT, int StackCapacityT>
class StackContainer 
{
public:
    using ContainerType = ContainerTypeT;
    using ContainedType = typename ContainerTypeT::value_type;
    using Allocator = StackAllocator<ContainedType, StackCapacityT>;

    StackContainer() 
    : mBuffer()
    , mAllocator(&mBuffer)
    , mContainer(mAllocator)
    {
        mContainer.reserve(StackCapacityT);
    }
    
    ContainerType& container() { return mContainer; }
    const ContainerType& container() const { return mContainer; }
    
    ContainerType* operator->() { return &mContainer; }
    const ContainerType* operator->() const { return &mContainer; }

protected:
    typename Allocator::Buffer mBuffer;
    Allocator mAllocator;
    ContainerType mContainer;

    StackContainer(const StackContainer&) = delete;
    StackContainer& operator=(const StackContainer&) = delete;
};

template<typename T, size_t StackCapacityT>
class StackVector : public StackContainer<std::vector<T, StackAllocator<T, StackCapacityT> >, StackCapacityT> 
{
public:
    using base = typename std::vector<T, StackAllocator<T, StackCapacityT>>;

    using value_type = typename base::value_type;
    using pointer = typename base::pointer;
    using const_pointer = typename base::const_pointer;
    using reference = typename base::reference;
    using const_reference = typename base::const_reference;
    using size_type = typename base::size_type;
    using difference_type = typename base::difference_type;

    using iterator = typename base::iterator;
    using const_iterator = typename base::const_iterator;
    using reverse_iterator = typename base::reverse_iterator;
    using const_reverse_iterator = typename base::const_reverse_iterator;


    StackVector() 
    : StackContainer<std::vector<T, StackAllocator<T, StackCapacityT> >, StackCapacityT>() 
    {
    }

    StackVector(const StackVector<T, StackCapacityT>& other)
    : StackContainer<std::vector<T, StackAllocator<T, StackCapacityT> >, StackCapacityT>() 
    {
       container().assign(other->begin(), other->end());
    }
    StackVector<T, StackCapacityT>& operator=(const StackVector<T, StackCapacityT>& other) 
    {
        container().assign(other->begin(), other->end());
        return *this;
    }

    StackVector(std::initializer_list<T>&& items)
    : StackContainer<std::vector<T, StackAllocator<T, StackCapacityT> >, StackCapacityT>()
    {
        insert(end(), items.begin(), items.end());
    }

    T& operator[](size_t i) 
    { 
        return container().operator[](i); 
    }

    const T& operator[](size_t i) const 
    {
        return container().operator[](i);
    }

    bool operator==(const StackVector& other) const
    {
        return container() == other.container();
    }

    bool operator!=(const StackVector& other) const
    {
        return container() != other.container();
    }

    bool operator<(const StackVector& other) const
    {
        return container() < other.container();
    }

    bool operator<=(const StackVector& other) const
    {
        return container() <= other.container();
    }

    bool operator>(const StackVector& other) const
    {
        return container() > other.container();
    }

    bool operator>=(const StackVector& other) const
    {
        return container() >= other.container();
    }

    reference front() { return container().front(); }
    const_reference front() const { return container().front(); }
    reference back() { return container().back(); }
    const_reference back() const { return container().back(); }

    pointer data() { return container().data(); }
    const_pointer data() const { return container().data(); }

    void push_back(const value_type& value) { container().push_back(value); }
    void push_back(value_type&& value) { container().push_back(std::forward<value_type&&>(value)); }

    void pop_back() { container().pop_back(); }

    iterator insert(const_iterator it, const value_type& value) { return container().insert(it, value); }
    iterator insert(const_iterator it, value_type&& value) { return container().insert(it, std::forward<value_type&&>(value)); }
    
    template<class IteratorT>
    iterator insert(const_iterator it, IteratorT first, IteratorT last) { return container().insert(it, first, last); }

    void swap(StackVector& other) { container().swap(other.container()); }

    iterator erase(const_iterator it) { return container().erase(it); }
    iterator erase(const_iterator first, const_iterator last) { return container().erase(first, last); }
    iterator swap_erase(const_iterator it)
    {
        if (empty())
        {
            return end();
        }

        if (it == (end() - 1))
        {
            container().pop_back();
            return end();
        }
        else
        {
            std::swap(*it, *back());
            container().pop_back();
            return it;
        }
    }

    void clear() { container().clear(); }
    void resize(size_type size) { container().resize(size); }
    void reserve(size_type size) { container().reserve(size); }

    bool empty() const { return container().empty(); }
    size_type size() const { return container().size(); }
    size_type capacity() const { return container().capacity(); }

    iterator begin() { return container().begin(); }
    const_iterator begin() const { return container().begin(); }

    iterator end() { return container().end(); }
    const_iterator end() const { return container().end(); }

    reverse_iterator rbegin() { return container().rbegin(); }
    const_reverse_iterator rbegin() const { return container().rbegin(); }

    reverse_iterator rend() { return container().rend(); }
    const_reverse_iterator  rend() const { return container().rend(); }
};

}