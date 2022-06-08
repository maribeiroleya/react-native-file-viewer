// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <functional>
#include <sstream>
#include <string>


#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Web.Http.h>
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
            void Initialize(ReactContext const& reactContext) noexcept
        {
            this->m_reactContext = reactContext;
        }



        winrt::Windows::Foundation::IAsyncAction openFileAsync(std::wstring filePath,
            winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValueObject> promise) noexcept
        {
            auto capturedPromise = promise;
            Windows::Storage::StorageFolder installFolder{ Windows::ApplicationModel::Package::Current().InstalledLocation() };
            Windows::Storage::StorageFile file{ co_await Windows::Storage::StorageFile::GetFileFromPathAsync(filePath) };
            if (file) {
                this->m_reactContext.UIDispatcher().Post([file, capturedPromise]()->winrt::fire_and_forget {
                    Windows::System::LauncherOptions launcherOptions;
                    launcherOptions.DisplayApplicationPicker(true);
                    bool success{ co_await Windows::System::Launcher::LaunchFileAsync(file) };
                    });
            }
            else {
                auto resultObject = winrt::Microsoft::ReactNative::JSValueObject();
                resultObject["false"] = true;
                capturedPromise.Resolve(resultObject);
            }
        }


        REACT_METHOD(open);
        void open(std::wstring filePath,
            winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValueObject> promise) noexcept
        {
            auto asyncOp = openFileAsync(filePath, promise);
            asyncOp.Completed([promise](auto action, auto status)
                {
                    if (status == winrt::Windows::Foundation::AsyncStatus::Error)
                    {
                        std::stringstream errorCode;
                        errorCode << "0x" << std::hex << action.ErrorCode() << std::endl;

                        auto error = winrt::Microsoft::ReactNative::ReactError();
                        error.Message = "HRESULT " + errorCode.str() + ": " + std::system_category().message(action.ErrorCode());
                        promise.Reject(error);
                    }
                });
        }


    private:
        React::ReactContext m_reactContext;
    };

}