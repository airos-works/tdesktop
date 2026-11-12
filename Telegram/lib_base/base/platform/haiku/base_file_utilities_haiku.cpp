// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/haiku/base_file_utilities_haiku.h"

#include "base/platform/base_platform_file_utilities.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>

#include "base/platform/haiku/base_haiku_begin.h"
#include <Entry.h>
#include <Messenger.h>
#include <Node.h>
#include <Path.h>
#include <Roster.h>
#include <image.h>
#include "base/platform/haiku/base_haiku_end.h"

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace base::Platform {
namespace {

constexpr auto kTrackerSignature = "application/x-vnd.Be-TRAK";
constexpr auto kTrackerSelect = 'Tsel';

// Opens the folder in Tracker with the file selected, as Tracker's own
// "Show in folder" does: B_REFS_RECEIVED with "nodeRefToSelect".
bool TrackerShowInFolder(const QString &filepath) {
	const auto path = QFile::encodeName(filepath);
	BEntry file(path.constData());
	entry_ref fileRef;
	BEntry folder;
	entry_ref folderRef;
	if (!file.Exists()
		|| file.GetRef(&fileRef) != B_OK
		|| file.GetParent(&folder) != B_OK
		|| folder.GetRef(&folderRef) != B_OK) {
		return false;
	}
	BMessage open(B_REFS_RECEIVED);
	open.AddRef("refs", &folderRef);
	node_ref fileNode;
	if (file.GetNodeRef(&fileNode) == B_OK) {
		open.AddData(
			"nodeRefToSelect",
			B_RAW_TYPE,
			&fileNode,
			sizeof(fileNode));
	}
	if (be_roster->IsRunning(kTrackerSignature)) {
		BMessenger tracker(kTrackerSignature);
		if (tracker.SendMessage(&open) == B_OK) {
			BMessage select(kTrackerSelect);
			select.AddRef("refs", &fileRef);
			tracker.SendMessage(&select);
			return true;
		}
	}
	return be_roster->Launch(&folderRef) == B_OK;
}

} // namespace

void ShowInFolder(const QString &filepath) {
	if (!TrackerShowInFolder(filepath)) {
		QDesktopServices::openUrl(
			QUrl::fromLocalFile(QFileInfo(filepath).absolutePath()));
	}
}

QString CurrentExecutablePath(int argc, char *argv[]) {
	// The team's application image is the executable, wherever it was
	// started from (Deskbar, Tracker, a link, the Terminal).
	auto info = image_info();
	auto cookie = int32(0);
	while (get_next_image_info(B_CURRENT_TEAM, &cookie, &info) == B_OK) {
		if (info.type == B_APP_IMAGE) {
			return QFile::decodeName(info.name);
		}
	}
	if (argc) {
		const auto argv0 = QFile::decodeName(argv[0]);
		if (!argv0.isEmpty() && !argv0.contains(QLatin1Char('/'))) {
			const auto inPath = QStandardPaths::findExecutable(argv0);
			if (!inPath.isEmpty()) {
				return inPath;
			}
		}
		return QFileInfo(argv0).absoluteFilePath();
	}
	return QString();
}

void RemoveQuarantine(const QString &path) {
}

QString BundledResourcesPath() {
	Unexpected("BundledResourcesPath not implemented.");
}

QString FileNameFromUserString(QString name) {
	return name;
}

bool DeleteDirectory(QString path) {
	if (path.endsWith('/')) {
		path.chop(1);
	}
	const auto pathRaw = QFile::encodeName(path);
	const auto d = opendir(pathRaw.constData());
	if (!d) {
		return false;
	}
	while (struct dirent *p = readdir(d)) {
		if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) {
			continue;
		}
		const auto fname = path + '/' + QFile::decodeName(p->d_name);
		const auto encoded = QFile::encodeName(fname);
		struct stat statbuf;
		if (!lstat(encoded.constData(), &statbuf)) {
			if (S_ISDIR(statbuf.st_mode)) {
				if (!DeleteDirectory(fname)) {
					closedir(d);
					return false;
				}
			} else if (unlink(encoded.constData())) {
				closedir(d);
				return false;
			}
		}
	}
	closedir(d);
	return !rmdir(pathRaw.constData());
}

bool RenameWithOverwrite(const QString &from, const QString &to) {
	const auto fromPath = QFile::encodeName(from);
	const auto toPath = QFile::encodeName(to);
	return (rename(fromPath.constData(), toPath.constData()) == 0);
}

void FlushFileData(QFile &file) {
	file.flush();
	if (const auto descriptor = file.handle(); descriptor >= 0) {
		fsync(descriptor);
	}
}

} // namespace base::Platform
