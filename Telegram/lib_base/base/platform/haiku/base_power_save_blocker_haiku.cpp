// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/haiku/base_power_save_blocker_haiku.h"

#include <QtGui/QWindow>

// air/OS has no API for an application to hold off the screen blanker yet;
// calls keep their bookkeeping and do nothing.

namespace base::Platform {

void BlockPowerSave(
		PowerSaveBlockType type,
		const QString &description,
		QPointer<QWindow> window) {
}

void UnblockPowerSave(PowerSaveBlockType type, QPointer<QWindow> window) {
}

} // namespace base::Platform
