/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "platform/haiku/file_utilities_haiku.h"

#include <QtCore/QFile>

#include "base/platform/haiku/base_haiku_begin.h"
#include <Entry.h>
#include <Messenger.h>
#include <Mime.h>
#include <Roster.h>
#include "base/platform/haiku/base_haiku_end.h"

namespace Platform {
namespace File {
namespace Unfused {

// Tracker's "Open with…" panel for the file, as its context menu has.
bool UnsafeShowOpenWith(const QString &filepath) {
	const auto path = QFile::encodeName(filepath);
	update_mime_info(path.constData(), false, true, false);
	BEntry entry(path.constData());
	entry_ref ref;
	if (entry.GetRef(&ref) != B_OK) {
		return false;
	}
	BMessenger tracker("application/x-vnd.Be-TRAK");
	if (!tracker.IsValid()) {
		return false;
	}
	BMessage open(B_REFS_RECEIVED);
	open.AddRef("refs", &ref);
	open.AddInt32("launchUsingSelector", 0);
	return tracker.SendMessage(&open) == B_OK;
}

} // namespace Unfused
} // namespace File
} // namespace Platform
