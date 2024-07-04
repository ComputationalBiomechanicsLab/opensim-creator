#pragma once

namespace osc
{
    // returned by `App::do_main_loop_step`
    //
    // callers should interpret an implicit conversion of this class to `true` as "the
    // tick was ok". An impicit conversion to `false` should be interpreted as "something
    // happened, you should stop stepping and maybe teardown the application loop"
    class AppMainLoopStatus final {
    public:
        // returns a status that means "the step was ok, feel free to keep stepping"
        static AppMainLoopStatus ok() { return Status::Ok; }

        // returns a status that means "something _requested_ that you stop stepping"
        //
        // (whether you stop or not is up to you - but you should probably stop)
        static AppMainLoopStatus quit_requested() { return Status::QuitRequested; }

        operator bool () const { return status_ == Status::Ok; }

    private:
        enum class Status { Ok, QuitRequested };

        AppMainLoopStatus(Status status) : status_{status} {}

        Status status_;
    };
}
