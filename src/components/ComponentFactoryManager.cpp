#include "ComponentFactoryManager.h"

#include <new>
#include <type_traits>

namespace zen
{
    namespace
    {
        static unsigned int g_niftyCounter = 0;
        static std::aligned_storage_t<sizeof(ComponentFactoryManager), alignof(ComponentFactoryManager)> g_singletonBuffer;
        ComponentFactoryManager& g_singleton = reinterpret_cast<ComponentFactoryManager&>(g_singletonBuffer);
    }

    IAutoComponentFactoryRegistry* ComponentFactoryManager::head = nullptr;
    IAutoComponentFactoryRegistry* ComponentFactoryManager::tail = nullptr;

    ComponentFactoryManagerInitializer::ComponentFactoryManagerInitializer()
    {
        if (g_niftyCounter++ == 0)
            new (&g_singleton) ComponentFactoryManager();
    }

    ComponentFactoryManagerInitializer::~ComponentFactoryManagerInitializer()
    {
        if (--g_niftyCounter == 0)
            (&g_singleton)->~ComponentFactoryManager();
    }

    ComponentFactoryManager& ComponentFactoryManager::get() noexcept
    {
        return g_singleton;
    }

    void ComponentFactoryManager::initialize()
    {
        for (auto it = head; it != nullptr; it = it->next())
            it->initialize();
    }

    std::optional<const IComponentFactory* const> ComponentFactoryManager::getFactory(std::string_view key) const
    {
        auto it = m_factories.find(key);
        if (it == m_factories.cend())
            return std::nullopt;

        return it->second.get();
    }
    
    bool ComponentFactoryManager::registerComponentFactory(std::string_view key, std::unique_ptr<class IComponentFactory> factory) noexcept
    {
        auto result = m_factories.emplace(key, std::move(factory));
        return result.second;
    }
}