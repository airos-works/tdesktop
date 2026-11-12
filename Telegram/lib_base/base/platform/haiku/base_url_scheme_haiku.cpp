// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/haiku/base_url_scheme_haiku.h"

#include <QtCore/QFile>

#include "base/platform/haiku/base_haiku_begin.h"
#include <AppFileInfo.h>
#include <Application.h>
#include <Entry.h>
#include <File.h>
#include <Mime.h>
#include <Roster.h>
#include "base/platform/haiku/base_haiku_end.h"

// air/OS opens a "scheme:" link by launching the preferred application of
// the MIME type application/x-vnd.Be.URL.<scheme> with the URL as argv[1]
// (B_ARGV_RECEIVED when it already runs). Registering a scheme is making
// Telegram that type's preferred application.

namespace base::Platform {
namespace {

[[nodiscard]] QByteArray SchemeType(const UrlSchemeDescriptor &descriptor) {
	return "application/x-vnd.Be.URL." + descriptor.protocol.toLatin1();
}

[[nodiscard]] QByteArray OwnSignature() {
	app_info info;
	if (be_app && be_app->GetAppInfo(&info) == B_OK) {
		return QByteArray(info.signature);
	}
	return QByteArray();
}

[[nodiscard]] QByteArray ExecutableSignature(const QString &executable) {
	BFile file(QFile::encodeName(executable).constData(), B_READ_ONLY);
	BAppFileInfo info(&file);
	char signature[B_MIME_TYPE_LENGTH];
	if (info.InitCheck() == B_OK && info.GetSignature(signature) == B_OK) {
		return QByteArray(signature);
	}
	return QByteArray();
}

[[nodiscard]] QByteArray Signature(const UrlSchemeDescriptor &descriptor) {
	auto result = ExecutableSignature(descriptor.executable);
	return result.isEmpty() ? OwnSignature() : result;
}

} // namespace

bool CheckUrlScheme(const UrlSchemeDescriptor &descriptor) {
	const auto signature = Signature(descriptor);
	if (signature.isEmpty()) {
		return false;
	}
	BMimeType type(SchemeType(descriptor).constData());
	char preferred[B_MIME_TYPE_LENGTH];
	return type.IsInstalled()
		&& type.GetPreferredApp(preferred) == B_OK
		&& !signature.compare(preferred, Qt::CaseInsensitive);
}

void RegisterUrlScheme(const UrlSchemeDescriptor &descriptor) {
	const auto signature = Signature(descriptor);
	if (signature.isEmpty()) {
		return;
	}
	BMimeType type(SchemeType(descriptor).constData());
	if (!type.IsInstalled()) {
		type.Install();
	}
	const auto name = descriptor.protocolName.toUtf8();
	type.SetShortDescription(name.constData());
	type.SetLongDescription(name.constData());
	type.SetPreferredApp(signature.constData());
}

void UnregisterUrlScheme(const UrlSchemeDescriptor &descriptor) {
	BMimeType type(SchemeType(descriptor).constData());
	char preferred[B_MIME_TYPE_LENGTH];
	if (type.IsInstalled()
		&& type.GetPreferredApp(preferred) == B_OK
		&& !Signature(descriptor).compare(preferred, Qt::CaseInsensitive)) {
		type.SetPreferredApp(nullptr);
	}
}

} // namespace base::Platform
