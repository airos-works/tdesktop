// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "ui/platform/haiku/ui_utility_haiku.h"

#include <QtGui/QWindow>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

// air/OS windows have app_server's decorator (the tab): no client-side
// frames, margins or window menus to manage, and no compositing manager
// for translucent top-level windows.

namespace Ui {
namespace Platform {

bool IsApplicationActive() {
	return QApplication::activeWindow() != nullptr;
}

bool TranslucentWindowsSupported() {
	return false;
}

void IgnoreAllActivation(not_null<QWidget*> widget) {
}

void ClearTransientParent(not_null<QWidget*> widget) {
	if (const auto window = widget->windowHandle()) {
		window->setTransientParent(nullptr);
	}
}

void SetForeignTransientParent(
		not_null<QWidget*> widget,
		const ForeignParent &parent) {
}

bool WindowMarginsSupported() {
	return false;
}

void SetWindowMargins(not_null<QWidget*> widget, const QMargins &margins) {
}

void ShowWindowMenu(not_null<QWidget*> widget, const QPoint &point) {
}

} // namespace Platform
} // namespace Ui
