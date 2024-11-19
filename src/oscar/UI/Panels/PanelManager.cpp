#include "PanelManager.h"

#include <oscar/UI/Panels/Panel.h>
#include <oscar/UI/Panels/ToggleablePanelFlags.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    // an that the user can toggle in-place at runtime
    class ToggleablePanel final {
    public:
        ToggleablePanel(
            std::string_view name,
            std::function<std::shared_ptr<Panel>(std::string_view)> panel_constructor,
            ToggleablePanelFlags flags) :

            name_{name},
            panel_constructor_{std::move(panel_constructor)},
            flags_{flags},
            instance_{std::nullopt}
        {}

        CStringView name() const
        {
            return name_;
        }

        Panel* instance_or_nullptr()
        {
            return instance_ ? instance_->get() : nullptr;
        }

        bool is_enabled_by_default() const
        {
            return flags_ & ToggleablePanelFlags::IsEnabledByDefault;
        }

        bool is_activated() const
        {
            return instance_.has_value();
        }

        void activate()
        {
            if (not instance_) {
                instance_ = panel_constructor_(name_);
            }
        }

        void deactivate()
        {
            instance_.reset();
        }

        void toggle_activated()
        {
            if (instance_ and (*instance_)->is_open()) {
                (*instance_)->close();
                instance_.reset();
            }
            else {
                instance_ = panel_constructor_(name_);
                (*instance_)->open();
            }
        }

        void on_draw()
        {
            if (instance_) {
                (*instance_)->on_draw();
            }
        }

        // clear any instance data if the panel has been closed
        void garbage_collect()
        {
            if (instance_ and !(*instance_)->is_open()) {
                instance_.reset();
            }
        }

    private:
        std::string name_;
        std::function<std::shared_ptr<Panel>(std::string_view)> panel_constructor_;
        ToggleablePanelFlags flags_;
        std::optional<std::shared_ptr<Panel>> instance_;
    };

    class DynamicPanel final {
    public:
        DynamicPanel(
            std::string_view base_name,
            size_t instance_number,
            std::shared_ptr<Panel> instance) :

            spawner_id_{std::hash<std::string_view>{}(base_name)},
            instance_number_{instance_number},
            instance_{std::move(instance)}
        {
            instance_->open();
        }

        Panel* instance_or_nullptr()
        {
            return instance_.get();
        }

        size_t spawnable_panel_id() const
        {
            return spawner_id_;
        }

        size_t instance_number() const
        {
            return instance_number_;
        }

        CStringView name() const
        {
            return instance_->name();
        }

        bool is_open() const
        {
            return instance_->is_open();
        }

        void on_draw()
        {
            instance_->on_draw();
        }

    private:
        size_t spawner_id_;
        size_t instance_number_;
        std::shared_ptr<Panel> instance_;
    };

    // declaration for a panel that can spawn new dyanmic panels (above)
    class SpawnablePanel final {
    public:
        SpawnablePanel(
            std::string_view base_name,
            std::function<std::shared_ptr<Panel>(std::string_view)> panel_constructor,
            size_t num_initially_opened_panels) :

            base_name_{base_name},
            panel_constructor_{std::move(panel_constructor)},
            num_initially_opened_panels_{num_initially_opened_panels}
        {}

        size_t id() const
        {
            return std::hash<std::string_view>{}(base_name_);
        }

        CStringView base_name() const
        {
            return base_name_;
        }

        DynamicPanel spawn_dynamic_panel(size_t instance_number, std::string_view panel_name)
        {
            return DynamicPanel{
                base_name_,
                instance_number,
                panel_constructor_(panel_name),
            };
        }

        size_t num_initially_opened_panels() const
        {
            return num_initially_opened_panels_;
        }

    private:
        std::string base_name_;
        std::function<std::shared_ptr<Panel>(std::string_view)> panel_constructor_;
        size_t num_initially_opened_panels_;
    };
}

class osc::PanelManager::Impl final {
public:

    void register_toggleable_panel(
        std::string_view base_name,
        std::function<std::shared_ptr<Panel>(std::string_view)> panel_constructor,
        ToggleablePanelFlags flags)
    {
        toggleable_panels_.emplace_back(base_name, std::move(panel_constructor), flags);
    }

    void register_spawnable_panel(
        std::string_view base_name,
        std::function<std::shared_ptr<Panel>(std::string_view)> panel_constructor,
        size_t num_initially_opened_panels)
    {
        spawnable_panels_.emplace_back(base_name, std::move(panel_constructor), num_initially_opened_panels);
    }

    Panel* try_upd_panel_by_name(std::string_view name)
    {
        for (ToggleablePanel& panel : toggleable_panels_) {

            if (Panel* p = panel.instance_or_nullptr(); (p != nullptr) and p->name() == name) {
                return p;
            }
        }

        for (DynamicPanel& panel : dynamic_panels_) {

            if (Panel* p = panel.instance_or_nullptr(); (p != nullptr) and p->name() == name) {
                return p;
            }
        }

        return nullptr;
    }

    size_t num_toggleable_panels() const
    {
        return toggleable_panels_.size();
    }

    CStringView toggleable_panel_name(size_t pos) const
    {
        return toggleable_panels_.at(pos).name();
    }

    bool is_toggleable_panel_activated(size_t pos) const
    {
        return toggleable_panels_.at(pos).is_activated();
    }

    void set_toggleable_panel_activated(size_t pos, bool v)
    {
        ToggleablePanel& panel = toggleable_panels_.at(pos);
        if (panel.is_activated() != v) {
            panel.toggle_activated();
        }
    }

    void set_toggleable_panel_activated(std::string_view panel_name, bool v)
    {
        for (size_t i = 0; i < toggleable_panels_.size(); ++i) {
            if (toggleable_panels_[i].name() == panel_name) {
                set_toggleable_panel_activated(i, v);
                return;
            }
        }
    }

    void on_mount()
    {
        if (not first_mount_) {
            return;  // already mounted once
        }

        // initialize default-open tabs
        for (ToggleablePanel& panel : toggleable_panels_) {
            if (panel.is_enabled_by_default()) {
                panel.activate();
            }
        }

        // initialize dynamic tabs that have some "initial" number
        // of spawned tabs
        for (size_t ith_panel = 0; ith_panel < spawnable_panels_.size(); ++ith_panel) {

            SpawnablePanel& panel = spawnable_panels_[ith_panel];
            for (size_t i = 0; i < panel.num_initially_opened_panels(); ++i) {
                create_dynamic_panel(ith_panel);
            }
        }

        first_mount_ = false;
    }

    void on_unmount()
    {
        // noop: we only mount the panels once and never unmount them
    }

    void on_tick()
    {
        // garbage-collect toggleable panels
        for (ToggleablePanel& panel : toggleable_panels_) {
            panel.garbage_collect();
        }

        // garbage-collect non-open dynamic panels
        for (size_t i = 0; i < dynamic_panels_.size(); ++i) {
            if (not dynamic_panels_[i].is_open()) {
                dynamic_panels_.erase(dynamic_panels_.begin() + i);
                --i;
            }
        }
    }

    void on_draw()
    {
        // draw each toggleable panel
        for (ToggleablePanel& panel : toggleable_panels_) {
            if (panel.is_activated()) {
                panel.on_draw();
            }
        }

        // draw each (set of) dynamic panel(s)
        for (DynamicPanel& panel : dynamic_panels_) {
            panel.on_draw();
        }
    }

    size_t num_dynamic_panels() const
    {
        return dynamic_panels_.size();
    }

    CStringView dynamic_panel_name(size_t pos) const
    {
        return dynamic_panels_.at(pos).name();
    }

    void deactivate_dynamic_panel(size_t pos)
    {
        if (pos < dynamic_panels_.size()) {
            dynamic_panels_.erase(dynamic_panels_.begin() + pos);
        }
    }

    size_t num_spawnable_panels() const
    {
        return spawnable_panels_.size();
    }

    CStringView spawnable_panel_base_name(size_t pos) const
    {
        return spawnable_panels_.at(pos).base_name();
    }

    void create_dynamic_panel(size_t pos)
    {
        SpawnablePanel& spawnable = spawnable_panels_.at(pos);
        const size_t ith_instance = calc_dynamic_panel_instance_number(spawnable.id());
        const std::string panel_name = calc_panel_name(spawnable.base_name(), ith_instance);

        push_dynamic_panel(spawnable.spawn_dynamic_panel(ith_instance, panel_name));
    }

    std::string suggested_dynamic_panel_name(std::string_view base_name)
    {
        const size_t ith_instance = calc_dynamic_panel_instance_number(std::hash<std::string_view>{}(base_name));
        return calc_panel_name(base_name, ith_instance);
    }

    void push_dynamic_panel(std::string_view base_name, std::shared_ptr<Panel> panel)
    {
        const size_t ith_instance = calc_dynamic_panel_instance_number(std::hash<std::string_view>{}(base_name));

        push_dynamic_panel(DynamicPanel{base_name, ith_instance, std::move(panel)});
    }

private:
    size_t calc_dynamic_panel_instance_number(size_t spawnable_id)
    {
        // compute instance index such that it's the lowest non-colliding
        // number with the same spawnable panel

        std::vector<bool> used(dynamic_panels_.size());
        for (DynamicPanel& panel : dynamic_panels_) {
            if (panel.spawnable_panel_id() == spawnable_id) {
                const size_t instance_number = panel.instance_number();
                used.resize(max(instance_number, used.size()));
                used[instance_number] = true;
            }
        }
        return std::distance(used.begin(), rgs::find(used, false));
    }

    std::string calc_panel_name(std::string_view base_name, size_t ith_instance)
    {
        std::stringstream ss;
        ss << base_name << ith_instance;
        return std::move(ss).str();
    }

    void push_dynamic_panel(DynamicPanel p)
    {
        dynamic_panels_.push_back(std::move(p));

        // re-sort so that panels are clustered correctly
        rgs::sort(dynamic_panels_, rgs::less{}, [](auto const& p)
        {
            return std::make_tuple(p.spawnable_panel_id(), p.instance_number());
        });
    }

    std::vector<ToggleablePanel> toggleable_panels_;
    std::vector<DynamicPanel> dynamic_panels_;
    std::vector<SpawnablePanel> spawnable_panels_;
    bool first_mount_ = true;
};


osc::PanelManager::PanelManager() :
    impl_{std::make_unique<Impl>()}
{}
osc::PanelManager::PanelManager(PanelManager&&) noexcept = default;
osc::PanelManager& osc::PanelManager::operator=(PanelManager&&) noexcept = default;
osc::PanelManager::~PanelManager() noexcept = default;

void osc::PanelManager::register_toggleable_panel(
    std::string_view base_name,
    std::function<std::shared_ptr<Panel>(std::string_view)> panel_constructor,
    ToggleablePanelFlags flags)
{
    impl_->register_toggleable_panel(base_name, std::move(panel_constructor), flags);
}

void osc::PanelManager::register_spawnable_panel(
    std::string_view base_name,
    std::function<std::shared_ptr<Panel>(std::string_view)> panel_constructor,
    size_t num_initially_opened_panels)
{
    impl_->register_spawnable_panel(base_name, std::move(panel_constructor), num_initially_opened_panels);
}

Panel* osc::PanelManager::try_upd_panel_by_name(std::string_view name)
{
    return impl_->try_upd_panel_by_name(name);
}

size_t osc::PanelManager::num_toggleable_panels() const
{
    return impl_->num_toggleable_panels();
}

CStringView osc::PanelManager::toggleable_panel_name(size_t pos) const
{
    return impl_->toggleable_panel_name(pos);
}

bool osc::PanelManager::is_toggleable_panel_activated(size_t pos) const
{
    return impl_->is_toggleable_panel_activated(pos);
}

void osc::PanelManager::set_toggleable_panel_activated(size_t pos, bool v)
{
    impl_->set_toggleable_panel_activated(pos, v);
}

void osc::PanelManager::set_toggleable_panel_activated(std::string_view panel_name, bool v)
{
    impl_->set_toggleable_panel_activated(panel_name, v);
}

void osc::PanelManager::on_mount()
{
    impl_->on_mount();
}

void osc::PanelManager::on_unmount()
{
    impl_->on_unmount();
}

void osc::PanelManager::on_tick()
{
    impl_->on_tick();
}

void osc::PanelManager::on_draw()
{
    impl_->on_draw();
}

size_t osc::PanelManager::num_dynamic_panels() const
{
    return impl_->num_dynamic_panels();
}

CStringView osc::PanelManager::dynamic_panel_name(size_t pos) const
{
    return impl_->dynamic_panel_name(pos);
}

void osc::PanelManager::deactivate_dynamic_panel(size_t pos)
{
    impl_->deactivate_dynamic_panel(pos);
}

size_t osc::PanelManager::num_spawnable_panels() const
{
    return impl_->num_spawnable_panels();
}

CStringView osc::PanelManager::spawnable_panel_base_name(size_t pos) const
{
    return impl_->spawnable_panel_base_name(pos);
}

void osc::PanelManager::create_dynamic_panel(size_t pos)
{
    impl_->create_dynamic_panel(pos);
}

std::string osc::PanelManager::suggested_dynamic_panel_name(std::string_view base_name)
{
    return impl_->suggested_dynamic_panel_name(base_name);
}

void osc::PanelManager::push_dynamic_panel(std::string_view base_name, std::shared_ptr<Panel> panel)
{
    impl_->push_dynamic_panel(base_name, std::move(panel));
}
