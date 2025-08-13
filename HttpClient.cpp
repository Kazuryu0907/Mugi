#include "pch.h"
#include "HttpClient.h"
#include "logging.h"
#include "nlohmann/json.hpp"
#include <thread>
#include <queue>

#define TOS(i) std::to_string(i)

extern std::shared_ptr<CVarManagerWrapper> _globalCvarManager;
using json = nlohmann::json;

const std::string base_url = "https://moca-8f967-default-rtdb.asia-southeast1.firebasedatabase.app/";

// Global async management
HINTERNET hSession = NULL;
std::atomic<RequestId> nextRequestId{1};
std::unordered_map<RequestId, std::shared_ptr<AsyncHttpRequest>> activeRequests;
std::queue<CallbackExecution> pendingCallbacks;
std::mutex requestsMutex;
std::mutex callbackMutex;
const auto REQUEST_TIMEOUT = std::chrono::seconds(30);

// Helper function to queue callback execution
void queueCallback(HttpCallback callback, bool success, int statusCode, const std::string& response) {
	if (callback) {
		std::lock_guard<std::mutex> lock(callbackMutex);
		pendingCallbacks.push({callback, success, statusCode, response});
	}
}

// WinHTTP callback function
void CALLBACK WinHttpStatusCallback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength) {
	RequestId requestId = static_cast<RequestId>(dwContext);
	
	// Find and extract request info with minimal lock time
	std::shared_ptr<AsyncHttpRequest> request;
	{
		std::lock_guard<std::mutex> lock(requestsMutex);
		auto it = activeRequests.find(requestId);
		if (it == activeRequests.end()) return;
		request = it->second;
	}
	
	switch (dwInternetStatus) {
	case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
		// Request sent, now receive response
		if (!WinHttpReceiveResponse(request->hRequest, NULL)) {
			queueCallback(request->callback, false, 0, "Failed to receive response");
			request->cleanupHandles();
			// Clean up request
			std::lock_guard<std::mutex> lock(requestsMutex);
			activeRequests.erase(requestId);
		}
		break;
		
	case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE: {
		// Headers received, check status and read response body
		DWORD statusCode = 0;
		DWORD statusCodeSize = sizeof(statusCode);
		bool success = false;
		std::string responseBody;
		
		if (WinHttpQueryHeaders(request->hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &statusCode, &statusCodeSize, NULL)) {
			success = (statusCode >= 200 && statusCode < 300);
		}
		
		// Read response body to properly close connection
		DWORD bytesAvailable = 0;
		if (WinHttpQueryDataAvailable(request->hRequest, &bytesAvailable)) {
			if (bytesAvailable > 0) {
				std::vector<char> buffer(bytesAvailable + 1);
				DWORD bytesRead = 0;
				if (WinHttpReadData(request->hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
					buffer[bytesRead] = '\0';
					responseBody = std::string(buffer.data(), bytesRead);
				}
			}
		}
		
		queueCallback(request->callback, success, statusCode, responseBody);
		request->cleanupHandles();
		
		// Clean up request
		std::lock_guard<std::mutex> lock(requestsMutex);
		activeRequests.erase(requestId);
		break;
	}
	
	case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR: {
		WINHTTP_ASYNC_RESULT* result = static_cast<WINHTTP_ASYNC_RESULT*>(lpvStatusInformation);
		std::string errorMsg = "Request error: " + std::to_string(result->dwError);
		queueCallback(request->callback, false, 0, errorMsg);
		request->cleanupHandles();
		
		// Clean up request
		std::lock_guard<std::mutex> lock(requestsMutex);
		activeRequests.erase(requestId);
		break;
	}
	
	// Handle additional error cases
	case WINHTTP_CALLBACK_STATUS_SECURE_FAILURE:
		queueCallback(request->callback, false, 0, "SSL/TLS error");
		request->cleanupHandles();
		{
			std::lock_guard<std::mutex> lock(requestsMutex);
			activeRequests.erase(requestId);
		}
		break;
		
	case WINHTTP_CALLBACK_STATUS_NAME_RESOLVED:
	case WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER:
	case WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER:
		// These are informational, don't need action
		break;
		
	default:
		break;
	}
}

bool initWinHttp() {
	hSession = WinHttpOpen(L"Mugi/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
	if (!hSession) {
		_globalCvarManager->log("Failed to initialize WinHTTP session");
		return false;
	}
	
	// Set the callback function
	if (WinHttpSetStatusCallback(hSession, WinHttpStatusCallback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0) == WINHTTP_INVALID_STATUS_CALLBACK) {
		_globalCvarManager->log("Failed to set WinHTTP callback");
		WinHttpCloseHandle(hSession);
		hSession = NULL;
		return false;
	}
	
	return true;
}

bool closeWinHttp() {
	cancelAllRequests();
	if (hSession) {
		bool result = WinHttpCloseHandle(hSession);
		hSession = NULL;
		return result;
	}
	return true;
}

bool httpPatchFirebase(const std::string& url, const std::string& method, const std::string& jsonData) {
	HINTERNET hRequest = NULL;
	HINTERNET hConnect = NULL;
	bool success = false;

	try {
		// Parse URL to extract components
		std::wstring wUrl(url.begin(), url.end());
		std::wstring wMethod(method.begin(), method.end());
		URL_COMPONENTS urlComp = { 0 };
		urlComp.dwStructSize = sizeof(urlComp);

		// Set buffer sizes
		WCHAR hostname[256] = { 0 };
		WCHAR path[1024] = { 0 };
		urlComp.lpszHostName = hostname;
		urlComp.dwHostNameLength = sizeof(hostname) / sizeof(WCHAR);
		urlComp.lpszUrlPath = path;
		urlComp.dwUrlPathLength = sizeof(path) / sizeof(WCHAR);

		
		if (!WinHttpCrackUrl(wUrl.c_str(), 0, 0, &urlComp)) {
			_globalCvarManager->log("Failed to parse URL");
			return false;
		}

		// Initialize WinHTTP
		// hSession = WinHttpOpen(L"Mugi/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
		// if (!hSession) {
		// 	_globalCvarManager->log("Failed to initialize WinHTTP session");
		// 	return false;
		// }

		// Connect to server
		hConnect = WinHttpConnect(hSession, hostname, urlComp.nPort, 0);
		if (!hConnect) {
			_globalCvarManager->log("Failed to connect to server");
			return false;
		}

		// Create request
		DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
		hRequest = WinHttpOpenRequest(hConnect, wMethod.c_str(), path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
		if (!hRequest) {
			_globalCvarManager->log("Failed to create HTTP request");
			return false;
		}

		// Set headers
		std::wstring headers = L"Content-Type: application/json\r\n";
		if (!WinHttpAddRequestHeaders(hRequest, headers.c_str(), -1L, WINHTTP_ADDREQ_FLAG_ADD)) {
			_globalCvarManager->log("Failed to add headers");
			return false;
		}

		// Send request
		if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)jsonData.c_str(), jsonData.length(), jsonData.length(), 0)) {
			_globalCvarManager->log("Failed to send HTTP request");
			return false;
		}

		// Receive response
		if (!WinHttpReceiveResponse(hRequest, NULL)) {
			_globalCvarManager->log("Failed to receive HTTP response");
			return false;
		}

		// Check status code
		DWORD statusCode = 0;
		DWORD statusCodeSize = sizeof(statusCode);
		if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &statusCode, &statusCodeSize, NULL)) {
			if (statusCode >= 200 && statusCode < 300) {
				success = true;
				// _globalCvarManager->log("Firebase PATCH successful, status: " + TOS(statusCode));
			} else {
				_globalCvarManager->log("Firebase PATCH failed, status: " + TOS(statusCode));
			}
		}

	} catch (...) {
		_globalCvarManager->log("Exception occurred during HTTP request");
	}

	// Cleanup
	if (hRequest) WinHttpCloseHandle(hRequest);
	if (hConnect) WinHttpCloseHandle(hConnect);
	// if (hSession) WinHttpCloseHandle(hSession);

	return success;
}

// Async HTTP functions
RequestId httpPatchFirebaseAsync(const std::string& url, const std::string& method, const std::string& jsonData, HttpCallback callback) {
	// Check if WinHTTP session is initialized
	if (!hSession) {
		_globalCvarManager->log("WinHTTP session not initialized");
		if (callback) callback(false, 0, "WinHTTP session not initialized");
		return 0;
	}
	
	RequestId requestId = nextRequestId++;
	auto request = std::make_shared<AsyncHttpRequest>();
	request->id = requestId;
	request->url = url;
	request->method = method;
	request->data = jsonData;
	request->callback = callback;
	request->startTime = std::chrono::steady_clock::now();
	
	try {
		// Parse URL to extract components
		std::wstring wUrl(url.begin(), url.end());
		std::wstring wMethod(method.begin(), method.end());
		URL_COMPONENTS urlComp = { 0 };
		urlComp.dwStructSize = sizeof(urlComp);

		// Set buffer sizes
		WCHAR hostname[256] = { 0 };
		WCHAR path[1024] = { 0 };
		urlComp.lpszHostName = hostname;
		urlComp.dwHostNameLength = sizeof(hostname) / sizeof(WCHAR);
		urlComp.lpszUrlPath = path;
		urlComp.dwUrlPathLength = sizeof(path) / sizeof(WCHAR);

		if (!WinHttpCrackUrl(wUrl.c_str(), 0, 0, &urlComp)) {
			_globalCvarManager->log("Failed to parse URL");
			if (callback) callback(false, 0, "Failed to parse URL");
			return 0;
		}

		// Connect to server
		request->hConnect = WinHttpConnect(hSession, hostname, urlComp.nPort, 0);
		if (!request->hConnect) {
			_globalCvarManager->log("Failed to connect to server");
			if (callback) callback(false, 0, "Failed to connect to server");
			return 0;
		}

		// Create request
		DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
		request->hRequest = WinHttpOpenRequest(request->hConnect, wMethod.c_str(), path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
		if (!request->hRequest) {
			_globalCvarManager->log("Failed to create HTTP request");
			request->cleanupHandles();
			if (callback) callback(false, 0, "Failed to create HTTP request");
			return 0;
		}

		// Set headers
		std::wstring headers = L"Content-Type: application/json\r\n";
		if (!WinHttpAddRequestHeaders(request->hRequest, headers.c_str(), -1L, WINHTTP_ADDREQ_FLAG_ADD)) {
			_globalCvarManager->log("Failed to add headers");
			request->cleanupHandles();
			if (callback) callback(false, 0, "Failed to add headers");
			return 0;
		}

		// Add request to active list before sending
		{
			std::lock_guard<std::mutex> lock(requestsMutex);
			activeRequests[requestId] = request;
		}

		// Send request asynchronously (use stored data, not parameter)
		if (!WinHttpSendRequest(request->hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)request->data.c_str(), request->data.length(), request->data.length(), requestId)) {
			_globalCvarManager->log("Failed to send HTTP request");
			{
				std::lock_guard<std::mutex> lock(requestsMutex);
				activeRequests.erase(requestId);
			}
			request->cleanupHandles();
			if (callback) callback(false, 0, "Failed to send HTTP request");
			return 0;
		}

		return requestId;
	} catch (...) {
		_globalCvarManager->log("Exception occurred during async HTTP request");
		if (callback) callback(false, 0, "Exception occurred");
		return 0;
	}
}

void processAsyncRequests() {
	// Process pending callbacks first (safe to execute on main thread)
	std::queue<CallbackExecution> callbacksToExecute;
	{
		std::lock_guard<std::mutex> lock(callbackMutex);
		callbacksToExecute = std::move(pendingCallbacks);
		pendingCallbacks = std::queue<CallbackExecution>(); // Clear original queue
	}
	
	// Execute callbacks with exception safety
	while (!callbacksToExecute.empty()) {
		auto& callbackExec = callbacksToExecute.front();
		try {
			if (callbackExec.callback) {
				callbackExec.callback(callbackExec.success, callbackExec.statusCode, callbackExec.response);
			}
		} catch (const std::exception& e) {
			_globalCvarManager->log("Exception in HTTP callback: " + std::string(e.what()));
		} catch (...) {
			_globalCvarManager->log("Unknown exception in HTTP callback");
		}
		callbacksToExecute.pop();
	}
	
	// Handle timeouts
	std::vector<RequestId> timedOutRequests;
	{
		std::lock_guard<std::mutex> lock(requestsMutex);
		auto now = std::chrono::steady_clock::now();
		
		for (auto it = activeRequests.begin(); it != activeRequests.end();) {
			auto& request = it->second;
			if (now - request->startTime > REQUEST_TIMEOUT) {
				_globalCvarManager->log("Request " + TOS(request->id) + " timed out");
				// Queue timeout callback
				queueCallback(request->callback, false, 0, "Request timed out");
				
				// Clean up handles safely
				request->cleanupHandles();
				it = activeRequests.erase(it);
			} else {
				++it;
			}
		}
	}
}

void cancelRequest(RequestId id) {
	std::lock_guard<std::mutex> lock(requestsMutex);
	auto it = activeRequests.find(id);
	if (it != activeRequests.end()) {
		auto& request = it->second;
		request->cleanupHandles();
		activeRequests.erase(it);
	}
}

void cancelAllRequests() {
	std::lock_guard<std::mutex> lock(requestsMutex);
	for (auto& pair : activeRequests) {
		auto& request = pair.second;
		request->cleanupHandles();
	}
	activeRequests.clear();
}

bool fb_update_time(int time) {
	json fb_data;
	fb_data["time"] = time;
	return httpPatchFirebase(base_url + "time.json", "PATCH", fb_data.dump());
}

bool fb_update_status(const std::string& status) {
	json fb_data;
	fb_data["status"] = status;
	return httpPatchFirebase(base_url + "status.json", "PATCH", fb_data.dump());
}

// Firebase async versions
RequestId fb_update_time_async(int time, HttpCallback callback) {
	json fb_data;
	fb_data["time"] = time;
	return httpPatchFirebaseAsync(base_url + "time.json", "PATCH", fb_data.dump(), callback);
}

RequestId fb_update_status_async(const std::string& status, HttpCallback callback) {
	json fb_data;
	fb_data["status"] = status;
	return httpPatchFirebaseAsync(base_url + "status.json", "PATCH", fb_data.dump(), callback);
}

RequestId fb_update_boost_async(int index, int boost, HttpCallback callback) {
	json fb_data;
	fb_data["index"] = index;
	fb_data["boost"] = boost;
	return httpPatchFirebaseAsync(base_url + "boost.json", "PATCH", fb_data.dump(), callback);
}
