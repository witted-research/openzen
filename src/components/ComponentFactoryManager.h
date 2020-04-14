//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_COMPONENTS_COMPONENTFACTORYMANAGER_H_
#define ZEN_COMPONENTS_COMPONENTFACTORYMANAGER_H_

#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>

#include "components/IComponentFactory.h"

namespace zen
{
    class ComponentFactoryManager
    {
    public:
        static ComponentFactoryManager& get() noexcept;

        void initialize();

        std::optional<const IComponentFactory* const> getFactory(std::string_view key) const;

        bool registerComponentFactory(std::string_view key, std::unique_ptr<IComponentFactory> factory) noexcept;

        static class IAutoComponentFactoryRegistry* head;
        static class IAutoComponentFactoryRegistry* tail;

    private:
        std::unordered_map<std::string_view, std::unique_ptr<IComponentFactory>> m_factories;
    };

    static struct ComponentFactoryManagerInitializer
    {
        ComponentFactoryManagerInitializer();
        ~ComponentFactoryManagerInitializer();
    } componentFactoryManagerInitializer;

    class IAutoComponentFactoryRegistry
    {
    public:
        IAutoComponentFactoryRegistry() noexcept
            : m_next(nullptr)
        {
            if (ComponentFactoryManager::tail == nullptr)
            {
                ComponentFactoryManager::head = ComponentFactoryManager::tail = this;
            }
            else
            {
                ComponentFactoryManager::tail->setNext(this);
                ComponentFactoryManager::tail = this;
            }
        }

        IAutoComponentFactoryRegistry* next() const noexcept { return m_next; }
        void setNext(IAutoComponentFactoryRegistry* next) noexcept { m_next = next; }

        virtual bool initialize() const noexcept = 0;

    private:
        IAutoComponentFactoryRegistry* m_next;
    };

    template <typename ComponentFactoryT>
    class AutoComponentFactoryRegistry : public IAutoComponentFactoryRegistry
    {
    public:
        AutoComponentFactoryRegistry(std::string_view key)
            : m_key(key)
        {}

        bool initialize() const noexcept override
        {
            return ComponentFactoryManager::get().registerComponentFactory(m_key, std::make_unique<ComponentFactoryT>());
        }

    private:
        std::string_view m_key;
    };

    template <typename ComponentFactoryT, typename... Args>
    AutoComponentFactoryRegistry<ComponentFactoryT> make_registry(Args&&... args)
    {
        return AutoComponentFactoryRegistry<ComponentFactoryT>(std::forward<Args>(args)...);
    }
}

#endif
