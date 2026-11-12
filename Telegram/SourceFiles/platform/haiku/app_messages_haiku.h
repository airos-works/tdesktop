/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

// Messages the roster and the notification server send Telegram's
// BApplication (Qt's): B_ARGV_RECEIVED when it is launched again with
// arguments (a tg:// link, a click on a notification, "Telegram -quit"),
// B_SILENT_RELAUNCH when it is launched again without. Telegram is a
// B_SINGLE_LAUNCH application on air/OS, so the second launch never
// starts a second Telegram: the roster hands its arguments over.

namespace Platform::AppMessages {

// The argument a notification's click adds:
// --airos-notification=<session>:<peer>:<topic root>:<monoforum peer>:<msg>
inline constexpr auto kNotificationArgument = "--airos-notification=";

struct NotificationClick {
	uint64 sessionId = 0;
	uint64 peerId = 0;
	int64 topicRootId = 0;
	uint64 monoforumPeerId = 0;
	int64 msgId = 0;
};

// Whether Qt's BApplication found Telegram running already: the roster
// has then given that instance this launch's arguments.
[[nodiscard]] bool AnotherInstanceRunning();

// Installs the filter on be_app (once; Qt must have created it).
void Start();

// The application's MIME signature (for BNotification::SetOnClickApp).
[[nodiscard]] QByteArray Signature();

// Notification clicks are reported here, on the main thread.
void SetNotificationHandler(Fn<void(NotificationClick)> handler);

} // namespace Platform::AppMessages
