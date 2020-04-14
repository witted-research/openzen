//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_UTILITY_LOCKINGQUEUE_H_
#define ZEN_UTILITY_LOCKINGQUEUE_H_

#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>

namespace zen
{
    template <typename T, typename Container = std::deque<T>>
    class LockingQueue
    {
    public:
        LockingQueue()
            : m_nWaiters(0)
            , m_terminate(false)
        {}

        ~LockingQueue()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_terminate = true;
            
            lock.unlock();
            m_cv.notify_all();

            lock.lock();
            m_cv.wait(lock, [this]() { return m_nWaiters == 0; });
        }

        void clear()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_container.clear();
            m_terminate = true;

            lock.unlock();
            m_cv.notify_all();

            lock.lock();
            m_cv.wait(lock, [this]() { return m_nWaiters == 0; });
            m_terminate = false;
        }

        void push(const T& value)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_container.push_back(value);
            m_cv.notify_one();
        }

        void push(T&& value)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_container.push_back(std::move(value));
            m_cv.notify_one();
        }

        template <class... Args>
        void emplace(Args&&... args)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_container.emplace_back(std::forward<Args>(args)...);
            m_cv.notify_one();
        }

        std::optional<T> tryToPop() noexcept
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_container.empty())
                return std::nullopt;

            std::optional<T> result(std::move(m_container.front()));
            m_container.pop_front();
            return result;
        }

        std::optional<T> waitToPop() noexcept
        {
            std::unique_lock<std::mutex> lock(m_mutex);

            ++m_nWaiters;
            m_cv.wait(lock, [this]() { return !m_container.empty() || m_terminate; });
            --m_nWaiters;

            if (m_terminate)
            {
                lock.unlock();
                m_cv.notify_all();
                return std::nullopt;
            }

            std::optional<T> result(std::move(m_container.front()));
            m_container.pop_front();
            return result;
        }

    private:
        Container m_container;
        std::condition_variable m_cv;
        std::mutex m_mutex;

        unsigned int m_nWaiters;
        bool m_terminate;
    };
}

#endif
