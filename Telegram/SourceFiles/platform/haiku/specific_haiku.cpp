/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "platform/haiku/specific_haiku.h"

#include "base/platform/base_platform_info.h"
#include "core/launcher.h"
#include "core/sandbox.h"
#include "core/application.h"
#include "core/update_checker.h"
#include "data/data_location.h"
#include "window/window_controller.h"
#include "window/main_window.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QSystemTrayIcon>
#include <QtGui/QDesktopServices>
#include <QtCore/QStandardPaths>

#include "base/platform/haiku/base_haiku_begin.h"
#include <FindDirectory.h>
#include <Path.h>
#include <Roster.h>
#include "base/platform/haiku/base_haiku_end.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <cstdio>
#include <unistd.h>

namespace {

constexpr auto kMediaPreferences = "application/x-vnd.Haiku-Media";

// air/OS starts what is in ~/config/settings/boot/launch at login.
[[nodiscard]] QString AutostartPath() {
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK) {
		return QString();
	}
	return QFile::decodeName(path.Path()) + u"/boot/launch/Telegram"_q;
}

bool WriteAutostart(bool enabled) {
	const auto path = AutostartPath();
	if (path.isEmpty()) {
		return false;
	}
	QFile file(path);
	if (!enabled) {
		return !file.exists() || file.remove();
	}
	QDir().mkpath(QFileInfo(path).absolutePath());
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		return false;
	}
	const auto executable = cExeDir() + cExeName();
	file.write("#!/bin/sh\n");
	file.write("exec \"" + QFile::encodeName(executable) + "\" -autostart\n");
	file.close();
	file.setPermissions(file.permissions()
		| QFileDevice::ExeOwner
		| QFileDevice::ExeGroup
		| QFileDevice::ExeOther);
	return true;
}

} // namespace

namespace Platform {

void SetApplicationIcon(const QIcon &icon) {
	QApplication::setWindowIcon(icon);
}

QString SingleInstanceLocalServerName(const QString &hash) {
	return QDir::tempPath()
		+ '/'
		+ hash
		+ '-'
		+ QCoreApplication::applicationName();
}

#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
std::optional<bool> IsDarkMode() {
	return std::nullopt;
}
#endif // Qt < 6.5.0

bool AutostartSupported() {
	return true;
}

void AutostartToggle(bool enabled, Fn<void(bool)> done) {
	const auto success = WriteAutostart(enabled);
	if (done) {
		done(enabled && success);
	}
}

bool AutostartSkip() {
	return !cAutoStart();
}

bool TrayIconSupported() {
	return QSystemTrayIcon::isSystemTrayAvailable();
}

bool SkipTaskbarSupported() {
	return false;
}

QString ExecutablePathForShortcuts() {
	return cExeDir() + cExeName();
}

} // namespace Platform

QString psAppDataPath() {
	// Where HaikuPorts' Telegram kept its data (~/config/settings/
	// TelegramDesktop): an existing session carries over.
	auto home = QDir::homePath();
	if (!home.isEmpty()) {
		auto oldPath = home + u"/.TelegramDesktop/"_q;
		auto oldSettingsBase = oldPath + u"tdata/settings"_q;
		if (QFile::exists(oldSettingsBase + '0')
			|| QFile::exists(oldSettingsBase + '1')
			|| QFile::exists(oldSettingsBase + 's')) {
			return oldPath;
		}
	}
	return QStandardPaths::writableLocation(
		QStandardPaths::AppConfigLocation) + '/';
}

int psCleanup() {
	Platform::AutostartToggle(false);
	return 0;
}

int psFixPrevious() {
	return 0;
}

namespace Platform {

void start() {
	QGuiApplication::setDesktopFileName(u"org.telegram.desktop"_q);
	LOG(("App ID: %1").arg(QGuiApplication::desktopFileName()));
}

void finish() {
}

PermissionStatus GetPermissionStatus(PermissionType type) {
	return PermissionStatus::Granted;
}

void RequestPermission(
		PermissionType type,
		Fn<void(PermissionStatus)> resultCallback) {
	resultCallback(PermissionStatus::Granted);
}

void OpenSystemSettingsForPermission(PermissionType type) {
	OpenSystemSettings(SystemSettingsType::Audio);
}

bool OpenSystemSettings(SystemSettingsType type) {
	if (type == SystemSettingsType::Audio) {
		const auto result = be_roster->Launch(kMediaPreferences);
		return (result == B_OK) || (result == B_ALREADY_RUNNING);
	}
	return true;
}

void NewVersionLaunched(int oldVersion) {
}

QImage DefaultApplicationIcon() {
	return Window::Logo();
}

QString ApplicationIconName() {
	return u"telegram"_q;
}

void LaunchMaps(const Data::LocationPoint &point, Fn<void()> fail) {
	const auto url = QUrl(
		u"https://www.openstreetmap.org/?mlat=%1&mlon=%2#map=16/%1/%2"_q.arg(
			point.latAsString(),
			point.lonAsString()));
	if (!QDesktopServices::openUrl(url)) {
		fail();
	}
}

namespace ThirdParty {

void start() {
}

void finish() {
}

} // namespace ThirdParty

} // namespace Platform

void psSendToMenu(bool send, bool silent) {
}

bool linuxMoveFile(const char *from, const char *to) {
	if (rename(from, to) == 0) {
		return true;
	}
	FILE *ffrom = fopen(from, "rb"), *fto = fopen(to, "wb");
	if (!ffrom) {
		if (fto) fclose(fto);
		return false;
	}
	if (!fto) {
		fclose(ffrom);
		return false;
	}
	static const int BufSize = 65536;
	char buf[BufSize];
	while (size_t size = fread(buf, 1, BufSize, ffrom)) {
		fwrite(buf, 1, size, fto);
	}
	struct stat fst;
	if (fstat(fileno(ffrom), &fst) == 0) {
		fchmod(fileno(fto), fst.st_mode);
	}
	fclose(ffrom);
	fclose(fto);
	return unlink(from) == 0;
}
