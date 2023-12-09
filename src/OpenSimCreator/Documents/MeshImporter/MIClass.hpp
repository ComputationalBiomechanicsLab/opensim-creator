#pragma once

#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <atomic>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

namespace osc::mi
{
    // a "class" for an object in the mesh importer document
    class MIClass final {
    public:
        MIClass(
            CStringView name,
            CStringView namePluralized,
            CStringView nameOptionallyPluralized,
            CStringView icon,
            CStringView description) :

            m_Data{std::make_shared<Data>(
                name,
                namePluralized,
                nameOptionallyPluralized,
                icon,
                description
            )}
        {
        }

        UID getID() const
        {
            return m_Data->id;
        }

        CStringView getName() const
        {
            return m_Data->name;
        }

        CStringView getNamePluralized() const
        {
            return m_Data->namePluralized;
        }

        CStringView getNameOptionallyPluralized() const
        {
            return m_Data->nameOptionallyPluralized;
        }

        CStringView getIconUTF8() const
        {
            return m_Data->icon;
        }

        CStringView getDescription() const
        {
            return m_Data->description;
        }

        // returns a unique string that can be used to name an instance of the given class
        std::string generateName() const
        {
            std::stringstream ss;
            ss << getName() << fetchAddUniqueCounter();
            return std::move(ss).str();
        }

        friend bool operator==(MIClass const& lhs, MIClass const& rhs)
        {
            return lhs.m_Data == rhs.m_Data || *lhs.m_Data == *rhs.m_Data;
        }
    private:
        int32_t fetchAddUniqueCounter() const
        {
            return m_Data->uniqueCounter.fetch_add(1, std::memory_order::relaxed);
        }

        struct Data final {
            Data(
                CStringView name_,
                CStringView namePluralized_,
                CStringView nameOptionallyPluralized_,
                CStringView icon_,
                CStringView description_) :

                name{name_},
                namePluralized{namePluralized_},
                nameOptionallyPluralized{nameOptionallyPluralized_},
                icon{icon_},
                description{description_}
            {
            }

            friend bool operator==(Data const&, Data const&) = default;

            UID id;
            std::string name;
            std::string namePluralized;
            std::string nameOptionallyPluralized;
            std::string icon;
            std::string description;
            mutable std::atomic<int32_t> uniqueCounter = 0;
        };

        std::shared_ptr<Data const> m_Data;
    };
}
