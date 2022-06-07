#pragma once

#include "JSValue.h"
#include "NativeModules.h"

using namespace winrt::Microsoft::ReactNative;

namespace winrt::ReactNativeFileViewer
{

REACT_MODULE(ReactNativeModule, L"ReactNativeFileViewer")
struct ReactNativeModule
{
    // See https://microsoft.github.io/react-native-windows/docs/native-modules for details on writing native modules

    REACT_INIT(Initialize)
    void Initialize(ReactContext const &reactContext) noexcept
    {
        m_reactContext = reactContext;
    }
    
    REACT_METHOD(open, L"open")
        Windows::Foundation::IAsyncAction open(std::string stringArgument, React::ReactPromise<std::string>&& result) noexcept
    {
        auto installFolder{ Windows::ApplicationModel::Package::Current().InstalledLocation() };

        Windows::Storage::StorageFile file{ co_await installFolder.GetFileAsync(stringArgument) };

        if (file)
        {
            // Launch the retrieved file
            bool success = co_await Windows::System::Launcher::LaunchFileAsync(file);
            if (success)
            {
                result.Resolve("ok");
            }
            else
            {
                result.Reject("file launch fail");
            }
        }
        else
        {
            result.Reject("file not found");
            // Could not find file
        }
       
    }

    private:
        ReactContext m_reactContext{nullptr};
};

} // namespace winrt::ReactNativeFileViewer
