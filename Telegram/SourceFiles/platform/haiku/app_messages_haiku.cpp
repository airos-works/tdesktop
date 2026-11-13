/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "platform/haiku/app_messages_haiku.h"

#include "platform/haiku/deskbar_protocol_haiku.h"

#include "core/sandbox.h"
#include "core/application.h"

#include <QtCore/QUrl>

#include "base/platform/haiku/base_haiku_begin.h"
#include <Application.h>
#include <MessageFilter.h>
#include <Roster.h>
#include "base/platform/haiku/base_haiku_end.h"

namespace Platform::AppMessages {
namespace {

Fn<void(NotificationClick)> NotificationHandler;
Fn<void(not_null<BMessage*>)> TrayHandler;

[[nodiscard]] std::optional<NotificationClick> ParseNotification(
		const QString &argument) {
	const auto prefix = QString::fromLatin1(kNotificationArgument);
	if (!argument.startsWith(prefix)) {
		return std::nullopt;
	}
	const auto parts = argument.mid(prefix.size()).split(':');
	if (parts.size() != 5) {
		return std::nullopt;
	}
	return NotificationClick{
		.sessionId = parts[0].toULongLong(),
		.peerId = parts[1].toULongLong(),
		.topicRootId = parts[2].toLongLong(),
		.monoforumPeerId = parts[3].toULongLong(),
		.msgId = parts[4].toLongLong(),
	};
}

// Runs on the Qt main thread.
void ProcessArguments(const QStringList &arguments, bool relaunch) {
	auto commands = QString();
	auto quit = false;
	for (const auto &argument : arguments) {
		if (const auto click = ParseNotification(argument)) {
			if (NotificationHandler) {
				NotificationHandler(*click);
			}
			return;
		} else if (argument == u"-quit"_q) {
			quit = true;
		} else if (argument.startsWith(u"tg:"_q)
			|| argument.startsWith(u"tonsite:"_q)
			|| argument.startsWith(u"http"_q)) {
			commands += u"OPEN:"_q
				+ QUrl(argument).toString(QUrl::FullyEncoded)
				+ ';';
		} else if (argument.startsWith(u"-"_q)) {
			continue;
		} else if (!argument.isEmpty()) {
			// A file Tracker dropped on the icon: Telegram sends files.
			commands += u"OPEN:"_q
				+ QUrl::fromLocalFile(argument).toString(QUrl::FullyEncoded)
				+ ';';
		}
	}
	if (quit) {
		commands += u"CMD:quit;"_q;
	} else if (commands.isEmpty() || relaunch) {
		commands += u"CMD:show;"_q;
	}
	Core::Sandbox::Instance().processExternalCommands(commands);
}

class RosterFilter final : public BMessageFilter {
public:
	RosterFilter() : BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE) {
	}

	filter_result Filter(BMessage *message, BHandler **target) override {
		switch (message->what) {
		case B_ARGV_RECEIVED: {
			if (!message->IsSourceRemote()) {
				// BApplication posts our own command line to itself:
				// Telegram has read it from main()'s arguments already.
				return B_SKIP_MESSAGE;
			}
			auto arguments = QStringList();
			const char *argument = nullptr;
			// argv[0] is the program
			for (auto i = 1
				; message->FindString("argv", i, &argument) == B_OK
				; ++i) {
				arguments.push_back(QString::fromUtf8(argument));
			}
			Post([=] { ProcessArguments(arguments, false); });
		} return B_SKIP_MESSAGE;
		default:
			if (DeskbarTray::IsTrayMessage(message->what)) {
				const auto copy = std::make_shared<BMessage>(*message);
				Post([=] {
					if (TrayHandler) {
						TrayHandler(copy.get());
					}
				});
				return B_SKIP_MESSAGE;
			}
			break;
		case B_SILENT_RELAUNCH:
			Post([] { ProcessArguments({}, true); });
			return B_SKIP_MESSAGE;
		}
		return B_DISPATCH_MESSAGE;
	}

private:
	// The roster's messages arrive on the BApplication thread.
	static void Post(Fn<void()> callback) {
		crl::on_main(std::move(callback));
	}

};

} // namespace

bool AnotherInstanceRunning() {
	return be_app && (be_app->InitCheck() == B_ALREADY_RUNNING);
}

void Start() {
	static auto started = false;
	if (started || !be_app) {
		return;
	}
	started = true;
	if (be_app->Lock()) {
		be_app->AddCommonFilter(new RosterFilter());
		be_app->Unlock();
	}
}

QByteArray Signature() {
	app_info info;
	if (be_app && be_app->GetAppInfo(&info) == B_OK) {
		return QByteArray(info.signature);
	}
	return QByteArray("application/x-vnd.telegram");
}

void SetNotificationHandler(Fn<void(NotificationClick)> handler) {
	NotificationHandler = std::move(handler);
}

void SetTrayHandler(Fn<void(not_null<BMessage*>)> handler) {
	TrayHandler = std::move(handler);
}

} // namespace Platform::AppMessages
