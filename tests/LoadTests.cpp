// Load the actual plugin without starting a server or authenticating a bot.
#include "SDK/API.h"
#include <cstdio>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

int main(int argc, char** argv)
{
    if (argc != 2) return 1;
#ifdef _WIN32
    const auto library = LoadLibraryA(argv[1]);
    const auto symbol = [&](const char* name) { return GetProcAddress(library, name); };
#else
    const auto library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    const auto symbol = [&](const char* name) { return dlsym(library, name); };
#endif
    if (!library) { std::puts("Could not load plugin library"); return 1; }
    const auto version = reinterpret_cast<uint32_t (*)()>(symbol("GetPluginApiVersion"));
    const auto signature = reinterpret_cast<uint64_t (*)()>(symbol("GetPluginSDKSignature"));
    const auto entry = reinterpret_cast<bool (*)(const PluginAPI*, void*)>(symbol("PluginMain"));
    const auto unload = reinterpret_cast<void (*)()>(symbol("PluginUnload"));
    const bool ok = version && signature && entry && unload
        && version() == PLUGIN_API_VERSION && signature() == PLUGIN_SDK_SIGNATURE
        && !entry(nullptr, nullptr);
    if (unload) unload();
#ifdef _WIN32
    FreeLibrary(library);
#else
    dlclose(library);
#endif
    if (!ok) std::puts("Plugin export or SDK compatibility check failed");
    return ok ? 0 : 1;
}
