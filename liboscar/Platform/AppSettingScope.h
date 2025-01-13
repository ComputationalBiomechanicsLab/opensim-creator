#pragma once

namespace osc
{
    // the "scope" of an application setting value
    //
    // this dictates if/how/when an application setting is read from, and written to,
    // storage
    enum class AppSettingScope {

        // the application setting value was set by either:
        //
        // - a readonly system-level configuration file
        // - runtime code (that doesn't want to persist/overwrite a user-level setting)
        //
        // when synchronizing, the implementation won't persist settings
        // with this scope
        System,

        // the application setting value was set by either:
        //
        // - a writable user-level configuration file
        // - runtime code
        //
        // when synchronizing, the implementation should try to
        // persist these settings to the user-level configuration
        // file
        User,

        NUM_OPTIONS,
    };
}
