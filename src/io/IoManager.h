//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_IO_IOMANAGER_H_
#define ZEN_IO_IOMANAGER_H_

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "ZenTypes.h"
#include "io/IIoSystem.h"

namespace zen
{
    namespace details
    {
        template <int... Is>
        struct index {};

        template <int N, int... Is>
        struct gen_seq : gen_seq<N - 1, N - 1, Is...> {};

        template <int... Is>
        struct gen_seq<0, Is...> : index<Is...> {};
    }

    class IAutoIoSystemRegistry;

    class IoManager
    {
    public:
        static IoManager& get() noexcept;

        void initialize() noexcept;

        bool registerIoSystem(std::string_view key, std::unique_ptr<IIoSystem> system) noexcept;

        std::optional<std::reference_wrapper<IIoSystem>> getIoSystem(std::string_view key) const noexcept;
        std::vector<std::reference_wrapper<IIoSystem>> getIoSystems() const noexcept;

        static IAutoIoSystemRegistry* head;
        static IAutoIoSystemRegistry* tail;

    private:
        std::unordered_map<std::string_view, std::unique_ptr<IIoSystem>> m_ioSystems;
        mutable std::mutex m_mutex;
    };

    static struct IoManagerInitializer
    {
        IoManagerInitializer();
        ~IoManagerInitializer();
    } ioManagerInitializer;

    class IAutoIoSystemRegistry
    {
    public:
        IAutoIoSystemRegistry() noexcept
        {
            if (IoManager::tail == nullptr)
            {
                IoManager::head = IoManager::tail = this;
            }
            else
            {
                IoManager::tail->setNext(this);
                IoManager::tail = this;
            }
        }

        virtual bool initialize() noexcept = 0;

        IAutoIoSystemRegistry* next() const noexcept { return m_next; }
        void setNext(IAutoIoSystemRegistry* next) noexcept { m_next = next; }

    private:
        IAutoIoSystemRegistry* m_next = nullptr;
    };

    template <typename SystemT, class... Args>
    class AutoIoSystemRegistry : public IAutoIoSystemRegistry
    {
    public:
        AutoIoSystemRegistry(Args&&... args) noexcept
            : m_args(std::forward<Args>(args)...)
        {
            static_assert(std::is_base_of_v<IIoSystem, SystemT>, "SystemT must derive from IIoSystem");

            constexpr auto KEY_LENGTH = sizeof(SystemT::KEY) / sizeof(*SystemT::KEY);
            constexpr auto MAX_LENGTH = sizeof(ZenSensorDesc::ioType) / sizeof(*ZenSensorDesc::ioType);
            static_assert(KEY_LENGTH <= MAX_LENGTH, "Key exceeds maximum length of characters.");
        }

        bool initialize() noexcept override
        {
            return initialize(m_args);
        }

    private:
        template <int... Is>
        bool initialize(std::tuple<Args...>& t, details::index<Is...>) noexcept
        {
            return IoManager::get().registerIoSystem(SystemT::KEY, std::make_unique<SystemT>(std::get<Is>(t)...));
        }

        bool initialize(std::tuple<Args...>& t) noexcept
        {
            return initialize(t, details::gen_seq<sizeof...(Args)>{});
        }

    private:
        std::tuple<Args...> m_args;
    };

    template <typename SystemT, typename... Args>
    AutoIoSystemRegistry<SystemT, Args...> makeRegistry(Args&&... args) noexcept
    {
        return AutoIoSystemRegistry<SystemT, Args...>(std::forward<Args>(args)...);
    }
}

#endif
