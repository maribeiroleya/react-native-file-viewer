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

#include <winrt/Microsoft.Web.WebView2.Core.h>

#include <kubazip/zip/zip.h>

#include <httplib.h>

using namespace winrt::Microsoft::ReactNative;
using namespace winrt::Microsoft::Web::WebView2::Core;

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

        char* wchar2char(wchar_t* bytes)
        {
            if (!bytes)
                return NULL;
#if defined(_WIN32) || defined(_MSC_VER)
            size_t size = WideCharToMultiByte(CP_UTF8, 0, bytes, -1, NULL, 0, NULL, NULL);
            if (!size)
                return NULL;
            char* res_str = (char*)calloc(size + 1, sizeof(char));
            if (!res_str)
                return NULL;
            if (!WideCharToMultiByte(CP_UTF8, 0, bytes, -1, res_str, size, NULL, NULL)) {
                free(res_str);
                res_str = NULL;
                return NULL;
            }
            return res_str;
#else
            char* res_str = (char*)calloc(ENCODE_MAX_SIZE + 1, sizeof(char));
            if (!res_str)
                return NULL;
            if (wcstombs(res_str, bytes, ENCODE_MAX_SIZE) == (size_t)-1) {
                free(res_str);
                res_str = NULL;
                return NULL;
            }
            return res_str;
#endif
        }

        winrt::Windows::Foundation::IAsyncAction unzipAsync(std::wstring filePath, std::wstring destPath,
            winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValueObject> promise) noexcept
        {
            auto capturedPromise = promise;

            winrt::apartment_context ui_thread; // Capture calling context.

            co_await winrt::resume_background();
            // Do compute-bound work here.
            int arg = 2;

            wchar_t w_filePath[260];
            wcscpy(w_filePath, filePath.c_str());

            wchar_t w_destPath[260];
            wcscpy(w_destPath, destPath.c_str());

            std::string destStr(destPath.begin(), destPath.end());
            char dest[200];
            strcpy_s(dest, destStr.c_str());

            bool haveLetters = false;
            struct zip_t* zip = zip_open(wchar2char(w_filePath), 0, 'r');
            int i, n = zip_entries_total(zip);

            for (i = 0; i < n; ++i) {
                zip_entry_openbyindex(zip, i);
                {
                    const char* name = zip_entry_name(zip);

                    //char teste[256];
                    //sprintf_s(teste, sizeof(teste), "UI: %d %d\n", strlen(name), strlen(dest));

                    //OutputDebugStringA(teste);

                    if (strlen(dest) + strlen(name) > 256) {
                        haveLetters = true;
                    }
                }
                zip_entry_close(zip);
            }
            zip_close(zip);

            if (!haveLetters) {
                zip_extract(wchar2char(w_filePath), wchar2char(w_destPath), 0, &arg);
                co_await ui_thread;

                auto resultObject = winrt::Microsoft::ReactNative::JSValueObject();
                resultObject["false"] = true;
                capturedPromise.Resolve(resultObject);
            }
            else {
                co_await ui_thread;

                auto error = winrt::Microsoft::ReactNative::ReactError();
                error.Message = "FILENAME IS TO BIG";
                promise.Reject(error);
            }   
        }



        REACT_METHOD(unzip);
        void unzip(std::wstring filePath, std::wstring destPath,
            winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValueObject> promise) noexcept
        {
            auto asyncOp = unzipAsync(filePath, destPath, promise);
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


        winrt::Windows::Foundation::IAsyncAction getWebView2VersionAsync(winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValueObject> promise) noexcept
        {
            auto capturedPromise = promise;
            auto resultObject = winrt::Microsoft::ReactNative::JSValueObject();
            winrt::hstring teste = winrt::to_hstring(CoreWebView2Environment::GetAvailableBrowserVersionString());

            resultObject["false"] = true;
            capturedPromise.Resolve(resultObject);
        }


        REACT_METHOD(getWebView2Version);
        void getWebView2Version(React::ReactPromise<std::string>&& promise) noexcept
        {
            try
            {
                winrt::hstring teste = winrt::to_hstring(CoreWebView2Environment::GetAvailableBrowserVersionString());
                promise.Resolve(winrt::to_string(teste));
            }
            catch (winrt::hresult_error const& ex) {
                promise.Resolve(winrt::to_string(L""));
            }
        }



        winrt::Windows::Foundation::IAsyncAction startServerAsync(std::wstring folderPath, int port) noexcept
        {
            winrt::apartment_context ui_thread; // Capture calling context.
            co_await winrt::resume_background();

            httplib::Server svr;
            std::string folderStr(folderPath.begin(), folderPath.end());
            char folder[200];
            strcpy_s(folder, folderStr.c_str());
            auto coiso = svr.set_mount_point("/", folder);
            svr.listen("localhost", port, 0);

            co_await ui_thread;
        }


        REACT_METHOD(startServer);
        void startServer(std::wstring filePath, int port) noexcept
        {
            startServerAsync(filePath, port);
        }


    private:
        React::ReactContext m_reactContext;
    };

}