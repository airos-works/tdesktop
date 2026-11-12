/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "platform/haiku/launcher_haiku.h"

#include "core/crash_reports.h"
#include "core/update_checker.h"

#include <QtCore/QProcess>

namespace Platform {

Launcher::Launcher(int argc, char *argv[])
: Core::Launcher(argc, argv) {
}

int Launcher::exec() {
	return Core::Launcher::exec();
}

// Packages update Telegram on air/OS (pkgman, SoftwareUpdater): the only
// launch asked of us is a relaunch, e.g. after a language or data change.
bool Launcher::launchUpdater(UpdaterLaunch action) {
	if (cExeName().isEmpty() || action != UpdaterLaunch::JustRelaunch) {
		return false;
	}
	auto arguments = QStringList();
	if (Logs::DebugEnabled()) {
		arguments.push_back(u"-debug"_q);
	}
	if (cLaunchMode() == LaunchModeAutoStart) {
		arguments.push_back(u"-autostart"_q);
	}
	if (cStartInTray()) {
		arguments.push_back(u"-startintray"_q);
	}
	if (cDataFile() != u"data"_q) {
		arguments.push_back(u"-key"_q);
		arguments.push_back(cDataFile());
	}
	arguments.push_back(u"-noupdate"_q);
	arguments.push_back(u"-tosettings"_q);
	if (customWorkingDir()) {
		arguments.push_back(u"-workdir"_q);
		arguments.push_back(cWorkingDir());
	}

	Logs::closeMain();
	CrashReports::Finish();

	return QProcess::startDetached(
		cExeDir() + cExeName(),
		arguments,
		initialWorkingDir());
}

} // namespace Platform
