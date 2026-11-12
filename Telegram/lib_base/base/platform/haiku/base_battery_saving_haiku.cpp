// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/haiku/base_battery_saving_haiku.h"

#include "base/battery_saving.h"

namespace base::Platform {
namespace {

class BatterySaving final : public AbstractBatterySaving {
public:
	std::optional<bool> enabled() const override {
		return std::nullopt;
	}

};

} // namespace

std::unique_ptr<AbstractBatterySaving> CreateBatterySaving(
		Fn<void()> changedCallback) {
	return std::make_unique<BatterySaving>();
}

} // namespace base::Platform
