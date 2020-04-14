//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_UTILITY_OWNERSHIP_H_
#define ZEN_UTILITY_OWNERSHIP_H_

#include <condition_variable>
#include <mutex>

namespace zen
{
    template <typename T>
    class Owner;

    template <typename T>
    class Borrowed
    {
    private:
        Owner<T>& m_owner;
        T& m_ref;

    public:
        Borrowed(Owner<T>& owner) noexcept
            : m_owner(owner)
            , m_ref(owner.m_data)
        {}

        ~Borrowed()
        {
            m_owner.release();
        }

        T& operator*() & noexcept { return m_ref; }
        T* operator->() const noexcept { return &m_ref; }
    };

    template <typename T>
    class Owner
    {
    public:
        friend class Borrowed<T>;

    private:
        T m_data;

        std::mutex m_mutex;
        std::condition_variable m_cv;

        bool m_borrowed;

    public:
        Owner(const Owner&) = delete;

        Owner(T&& data) noexcept
            : m_data(std::move(data))
            , m_borrowed(false)
        {}

        template <typename ...Args>
        Owner(Args&& ...args) noexcept
            : m_data(std::forward<Args>(args)...)
            , m_borrowed(false)
        {}

        Owner(Owner&& other) noexcept
            : m_data(std::move(other.m_data))
            , m_borrowed(other.m_borrowed)
        {}

        Owner& operator=(Owner&& other) noexcept
        {
            m_data = std::move(other.m_data);
            m_borrowed = other.m_obtained;
            return *this;
        }

        Borrowed<T> borrow() noexcept
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] { return !m_borrowed; });
            m_borrowed = true;

            return { *this };
        }

    private:
        void release() noexcept
        {
            m_borrowed = false;
            m_cv.notify_one();
        }
    };
}

#endif
