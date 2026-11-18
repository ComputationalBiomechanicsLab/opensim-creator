#pragma once

#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/uid.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

namespace osc
{
    // a "class" for an object in the mesh importer document
    class MiClass final {
    public:
        MiClass(
            CStringView name,
            CStringView name_pluralized,
            CStringView name_optionally_pluralized,
            CStringView icon,
            CStringView description) :

            data_{std::make_shared<Data>(
                name, name_pluralized, name_optionally_pluralized, icon, description)}
        {}

        UID get_id() const { return data_->id; }

        CStringView get_name() const { return data_->name; }

        CStringView get_name_pluralized() const { return data_->name_pluralized; }

        CStringView get_name_optionally_pluralized() const
        {
            return data_->name_optionally_pluralized;
        }

        CStringView get_icon_utf8() const { return data_->icon; }

        CStringView get_description() const { return data_->description; }

        // returns a unique string that can be used to name an instance of the given class
        std::string generate_name() const
        {
            std::stringstream ss;
            ss << get_name() << fetch_add_unique_counter();
            return std::move(ss).str();
        }

        friend bool operator==(const MiClass& lhs, const MiClass& rhs)
        {
            return lhs.data_ == rhs.data_ || *lhs.data_ == *rhs.data_;
        }

    private:
        int32_t fetch_add_unique_counter() const
        {
            return data_->unique_counter.fetch_add(1, std::memory_order::relaxed);
        }

        struct Data final {
            Data(
                CStringView name,
                CStringView name_pluralized,
                CStringView name_optionally_pluralized,
                CStringView icon,
                CStringView description) :

                name{name},
                name_pluralized{name_pluralized},
                name_optionally_pluralized{name_optionally_pluralized},
                icon{icon},
                description{description}
            {}

            friend bool operator==(const Data&, const Data&) = default;

            UID id;
            std::string name;
            std::string name_pluralized;
            std::string name_optionally_pluralized;
            std::string icon;
            std::string description;
            mutable std::atomic<int32_t> unique_counter = 0;
        };

        std::shared_ptr<const Data> data_;
    };
}
