#include <emscripten.h>

extern "C" int emscripten_main(int argc, char *argv[]);

namespace impl
{
EM_JS(void, glue_preinit, (), {
    var entry = __glue_main_;
    if (entry)
    {
        /*
         * None of the WebGPU properties appear to survive Closure, including
         * Emscripten's own `preinitializedWebGPUDevice` (which from looking at
         *`library_html5` is probably designed to be inited in script before
         * loading the Wasm).
         */
        if (navigator["gpu"])
        {
            navigator["gpu"]["requestAdapter"]().then(function(adapter) { adapter["requestDevice"]().then(function(device) {
                                                                              Module["preinitializedWebGPUDevice"] = device;
                                                                              entry();
                                                                          }); }, function() { console.error("No WebGPU adapter; not starting"); });
        }
        else
        {
            console.error("No support for WebGPU; not starting");
        }
    }
    else
    {
        console.error("Entry point not found; unable to start");
    }
});
} // namespace impl

__attribute__((used, visibility("default"))) extern "C" void _glue_main_()
{
    emscripten_main(0, nullptr);
}

int main(int argc, char *argv[])
{
    impl::glue_preinit();
    return 0;
}
