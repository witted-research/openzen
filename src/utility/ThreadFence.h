//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_UTILITY_THREADFENCE_H_
#define ZEN_UTILITY_THREADFENCE_H_

#include <condition_variable>
#include <mutex>

namespace zen
{
    class ThreadFence
    {
    public:
        ThreadFence()
            : m_terminated(false)
        {}

        /** Resets the state */
        void reset()
        {
            m_terminated = false;
        }

        /** Waits for the fence to be terminated */
        void wait()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] { return m_terminated; });
        }

        /** Waits for the fence to be terminated, or until the specified amount of time runs out
         * \param time The time to wait for termination
         * \return Whether the fence was terminated
         */
        template <typename Rep, typename Period>
        bool waitFor(const std::chrono::duration<Rep, Period>& time)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            return m_cv.wait_for(lock, time, [this] { return m_terminated; });
        }

        void terminate()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_terminated = true;
            m_cv.notify_all();
        }

    private:
        std::mutex m_mutex;
        std::condition_variable m_cv;
        bool m_terminated;
    };
}

#endif
