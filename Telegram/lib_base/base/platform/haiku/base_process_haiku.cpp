// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/haiku/base_process_haiku.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>

#include "base/platform/haiku/base_haiku_begin.h"
#include <Roster.h>
#include "base/platform/haiku/base_haiku_end.h"

namespace base::Platform {

// A pid is a team id on air/OS: the roster brings that team's frontmost
// window forward, as clicking it in Deskbar does.
void ActivateProcessWindow(int64 pid, WId windowId) {
	if (pid > 0) {
		be_roster->ActivateApp(team_id(pid));
	}
}

void ActivateThisProcessWindow(WId windowId) {
	for (const auto window : QGuiApplication::allWindows()) {
		if (window->winId() == windowId) {
			window->raise();
			window->requestActivate();
			return;
		}
	}
}

} // namespace base::Platform
