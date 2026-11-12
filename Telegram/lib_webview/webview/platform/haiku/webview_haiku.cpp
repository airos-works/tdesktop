// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "webview/platform/haiku/webview_haiku.h"

#include "base/random.h"

// air/OS has no embeddable web view for Qt windows: Telegram's mini apps,
// payments and passport open in the browser. The answer is immediate (the
// freedesktop build spawned a WebKitGTK helper here and waited up to five
// seconds for it on every start).

namespace Webview {

Available Availability() {
	return Available{
		.error = Available::Error::NoWebKitGTK,
		.details = "air/OS has no embedded web view for Telegram yet.",
	};
}

bool HiddenSupported() {
	return false;
}

bool SupportsEmbedAfterCreate() {
	return false;
}

bool SeparateStorageIdSupported() {
	return false;
}

std::unique_ptr<Interface> CreateInstance(Config config) {
	return nullptr;
}

std::string GenerateStorageToken() {
	constexpr auto kSize = 16;
	auto result = std::string(kSize, ' ');
	base::RandomFill(result.data(), result.size());
	return result;
}

void ClearStorageDataByToken(const std::string &token) {
}

} // namespace Webview
