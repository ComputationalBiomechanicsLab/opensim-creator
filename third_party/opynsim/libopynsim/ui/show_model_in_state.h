#pragma once

namespace opyn { class Model; }
namespace opyn { class ModelState; }

namespace opyn
{
    void show_model_in_state(
        const Model&,
        const ModelState&,
        bool zoom_to_fit
    );
}
