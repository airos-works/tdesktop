/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "webauthn/cable_scanner.h"

// air/OS: passkeys over caBLE need a BLE advertisement scanner. air/OS's
// Bluetooth LE stack has no scanning API for applications yet, so the
// scanner reports Bluetooth as unavailable and the hybrid flow offers the
// QR code path only.

namespace Platform::WebAuthn::Cable {
namespace {

class HaikuBleScanner final : public BleScanner {
public:
	bool start(
			std::function<void(QByteArray)> onAdvert,
			std::function<void(bool)> onAvailability) override {
		if (onAvailability) {
			onAvailability(false);
		}
		return false;
	}
	void stop() override {
	}
};

} // namespace

std::unique_ptr<BleScanner> MakeBleScanner() {
	return std::make_unique<HaikuBleScanner>();
}

} // namespace Platform::WebAuthn::Cable
