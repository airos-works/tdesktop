// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/haiku/base_info_haiku.h"

#include <QtCore/QJsonObject>
#include <QtCore/QLocale>
#include <QtCore/QDate>
#include <QtCore/QFile>

#include <sys/utsname.h>


namespace Platform {
namespace {

// "hrev60207+2559" from the kernel's version string
// ("hrev60207+2559 Nov  9 2026 16:21:02").
[[nodiscard]] QString KernelRevision() {
	struct utsname u;
	if (uname(&u) != 0) {
		return QString();
	}
	return QString::fromLatin1(u.version).split(' ').value(0);
}

} // namespace

QString DeviceModelPretty() {
	using namespace base::Platform;
	static const auto result = FinalizeDeviceModel([] {
		// air/OS has no DMI tables for applications to read; a laptop has
		// a battery, which is what Telegram's session list cares about.
		const auto battery = QFile::exists(u"/dev/power/acpi_battery/0"_q);
		return battery ? u"Laptop"_q : u"Desktop"_q;
	}());
	return result;
}

QString SystemVersionPretty() {
	static const auto result = [] {
		const auto revision = KernelRevision();
		return revision.isEmpty()
			? u"air/OS"_q
			: (u"air/OS "_q + revision);
	}();
	return result;
}

QString SystemCountry() {
	return QLocale::system().name().split('_').last();
}

QString SystemLanguage() {
	const auto system = QLocale::system();
	const auto languages = system.uiLanguages();
	return languages.isEmpty()
		? system.name().split('_').first()
		: languages.front();
}

QDate WhenSystemBecomesOutdated() {
	return QDate();
}

int AutoUpdateVersion() {
	return 4;
}

QString AutoUpdateKey() {
	return "haiku";
}

QString GetLibcName() {
	return u"libroot"_q;
}

QString GetLibcVersion() {
	return QString();
}

QString GetWindowManager() {
	return u"app_server"_q;
}

void Start(QJsonObject options) {
}

void Finish() {
}

} // namespace Platform
