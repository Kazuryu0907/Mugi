#pragma once

#include <string>
#include <functional>
#include <memory>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <windows.h>
#include <winhttp.h>

// Async HTTP request types
using HttpCallback = std::function<void(bool success, int statusCode, const std::string& response)>;
using RequestId = uint32_t;

// Callback execution data
struct CallbackExecution {
    HttpCallback callback;
    bool success;
    int statusCode;
    std::string response;
};

// Request data structure
struct AsyncHttpRequest {
    RequestId id;
    std::string url;
    std::string method;
    std::string data;
    HttpCallback callback;
    HINTERNET hRequest;
    HINTERNET hConnect;
    std::chrono::steady_clock::time_point startTime;
    std::atomic<bool> handlesCleanedUp{false};
    
    // Safe cleanup that prevents double cleanup
    void cleanupHandles() {
        bool expected = false;
        if (handlesCleanedUp.compare_exchange_strong(expected, true)) {
            // Only cleanup if we're the first to set the flag
            if (hRequest) {
                WinHttpCloseHandle(hRequest);
                hRequest = NULL;
            }
            if (hConnect) {
                WinHttpCloseHandle(hConnect);
                hConnect = NULL;
            }
        }
    }
};

// Original synchronous functions
bool initWinHttp();
bool closeWinHttp();
bool httpPatchFirebase(const std::string& url, const std::string& method, const std::string& jsonData);

// New async functions
RequestId httpPatchFirebaseAsync(const std::string& url, const std::string& method, const std::string& jsonData, HttpCallback callback = nullptr);
void processAsyncRequests();
void cancelRequest(RequestId id);
void cancelAllRequests();

// Firebase convenience functions (remain synchronous interface, async implementation)
bool fb_update_time(int time);
bool fb_update_status(const std::string& status);

// Firebase async versions
RequestId fb_update_time_async(int time, HttpCallback callback = nullptr);
RequestId fb_update_status_async(const std::string& status, HttpCallback callback = nullptr);
RequestId fb_update_boost_async(int index, int boost, HttpCallback callback = nullptr);
