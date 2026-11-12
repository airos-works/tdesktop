/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "platform/haiku/current_geo_location_haiku.h"

#include "core/current_geo_location.h"

// air/OS has no location service: Telegram asks for a place on the map.

namespace Platform {

void ResolveCurrentExactLocation(Fn<void(Core::GeoLocation)> callback) {
	callback({});
}

void ResolveLocationAddress(
		const Core::GeoLocation &location,
		const QString &language,
		Fn<void(Core::GeoAddress)> callback) {
	callback({});
}

} // namespace Platform
