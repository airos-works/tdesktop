// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/haiku/base_global_shortcuts_haiku.h"

#include "base/platform/base_platform_global_shortcuts.h"

#include <QtGui/QKeyEvent>
#include <QtGui/QKeySequence>

// air/OS delivers keys to the focused window only; system-wide hot keys
// belong to input_server filters (Shortcuts preferences), not applications.
// Push-to-talk works while a Telegram window is focused.

namespace base::Platform::GlobalShortcuts {

bool Available() {
	return false;
}

bool Allowed() {
	return false;
}

void Start(Fn<void(GlobalShortcutKeyGeneric descriptor, bool down)> process) {
}

void Stop() {
}

QString KeyName(GlobalShortcutKeyGeneric descriptor) {
	return QKeySequence(int(descriptor)).toString(QKeySequence::NativeText);
}

bool IsToggleFullScreenKey(not_null<QKeyEvent*> e) {
	// Command+Enter, as Haiku's own applications (MediaPlayer, Terminal)
	return (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)
		&& (e->modifiers() & Qt::ControlModifier);
}

} // namespace base::Platform::GlobalShortcuts
