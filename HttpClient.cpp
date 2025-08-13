#include "pch.h"
#include "HttpClient.h"
#include "logging.h"
#include "nlohmann/json.hpp"

#define TOS(i) std::to_string(i)

extern std::shared_ptr<CVarManagerWrapper> _globalCvarManager;
using json = nlohmann::json;

bool httpPatchFirebase(const std::string& url, const std::string& method, const std::string& jsonData) {
	HINTERNET hSession = NULL;
	HINTERNET hConnect = NULL;
	HINTERNET hRequest = NULL;
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
		hSession = WinHttpOpen(L"Mugi/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
		if (!hSession) {
			_globalCvarManager->log("Failed to initialize WinHTTP session");
			return false;
		}

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
				_globalCvarManager->log("Firebase PATCH successful, status: " + TOS(statusCode));
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
	if (hSession) WinHttpCloseHandle(hSession);

	return success;
}

bool fb_update_time(int time) {
	json fb_data;
	fb_data["time"] = time;
	return httpPatchFirebase("https://moca-8f967-default-rtdb.asia-southeast1.firebasedatabase.app/time.json", "PATCH", fb_data.dump());
}
