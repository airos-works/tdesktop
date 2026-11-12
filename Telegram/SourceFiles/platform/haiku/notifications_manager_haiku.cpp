/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "platform/haiku/notifications_manager_haiku.h"

#include "platform/haiku/app_messages_haiku.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "data/data_forum_topic.h"
#include "data/data_saved_sublist.h"
#include "history/history.h"
#include "history/history_item.h"
#include "main/main_session.h"
#include "window/notifications_utilities.h"

#include "base/platform/haiku/base_haiku_begin.h"
#include <Bitmap.h>
#include <Notification.h>
#include "base/platform/haiku/base_haiku_end.h"

// air/OS notifications are BNotifications: notification_server shows them
// in the desktop's notification window, with the sender's userpic, under a
// "Telegram" group. A click "launches" Telegram with an argument naming the
// message; Telegram being B_SINGLE_LAUNCH, the roster hands that argument
// to the running Telegram (B_ARGV_RECEIVED, platform/haiku/app_messages).

namespace Platform {
namespace Notifications {

bool Supported() {
	return true;
}

void Finish() {
}

bool SkipAudio() {
	return false;
}

bool SkipToast() {
	return false;
}

bool SkipFlashBounce() {
	return false;
}

bool SkipAudioForCustom() {
	return false;
}

bool SkipToastForCustom() {
	return false;
}

bool SkipFlashBounceForCustom() {
	return false;
}

bool Enforced() {
	return false;
}

bool ByDefault() {
	return true;
}

bool WaitForInputForCustom() {
	return false;
}

void MaybePlaySoundForCustom(Fn<void()> playSound) {
	playSound();
}

void MaybeFlashBounceForCustom(Fn<void()> flashBounce) {
	flashBounce();
}

bool VolumeSupported() {
	return false;
}

void Create(Window::Notifications::System *system) {
	if (Core::App().settings().nativeNotifications() && Supported()) {
		system->setManager([=] {
			return std::make_unique<Manager>(system);
		});
		return;
	}
	system->setManager(nullptr);
}

namespace {

// The message id of a notification: notification_server replaces a
// notification with the same id, so one per chat (topic) is kept.
[[nodiscard]] QByteArray MessageId(
		const Manager::ContextId &context) {
	return u"%1:%2:%3:%4"_q
		.arg(context.sessionId)
		.arg(context.peerId.value)
		.arg(context.topicRootId.bare)
		.arg(context.monoforumPeerId.value)
		.toLatin1();
}

[[nodiscard]] BBitmap *MakeIcon(const QImage &image) {
	if (image.isNull()) {
		return nullptr;
	}
	// B_RGBA32 is BGRA in memory, as QImage::Format_ARGB32 on
	// little-endian machines; neither is premultiplied.
	const auto argb = image.convertToFormat(QImage::Format_ARGB32);
	const auto result = new BBitmap(
		BRect(0, 0, argb.width() - 1, argb.height() - 1),
		B_RGBA32);
	if (result->InitCheck() != B_OK) {
		delete result;
		return nullptr;
	}
	const auto bytes = std::min(
		int(result->BytesPerRow()),
		int(argb.bytesPerLine()));
	auto to = static_cast<uchar*>(result->Bits());
	for (auto y = 0; y != argb.height(); ++y) {
		memcpy(to, argb.constScanLine(y), bytes);
		to += result->BytesPerRow();
	}
	return result;
}

} // namespace

class Manager::Private final {
public:
	explicit Private(not_null<Manager*> manager);
	~Private();

	void showNotification(
		NotificationInfo &&info,
		Ui::PeerUserpicView &userpicView);

private:
	const not_null<Manager*> _manager;
	const QByteArray _signature;

};

Manager::Private::Private(not_null<Manager*> manager)
: _manager(manager)
, _signature(AppMessages::Signature()) {
	AppMessages::Start();
	AppMessages::SetNotificationHandler([=](
			AppMessages::NotificationClick click) {
		_manager->notificationActivated({
			.contextId = {
				.sessionId = click.sessionId,
				.peerId = PeerId(click.peerId),
				.topicRootId = MsgId(click.topicRootId),
				.monoforumPeerId = PeerId(click.monoforumPeerId),
			},
			.msgId = MsgId(click.msgId),
		});
	});
}

Manager::Private::~Private() {
	AppMessages::SetNotificationHandler(nullptr);
}

void Manager::Private::showNotification(
		NotificationInfo &&info,
		Ui::PeerUserpicView &userpicView) {
	const auto peer = info.peer;
	const auto context = ContextId{
		.sessionId = peer->session().uniqueId(),
		.peerId = peer->id,
		.topicRootId = info.topicRootId,
		.monoforumPeerId = info.monoforumPeerId,
	};
	const auto title = info.subtitle.isEmpty()
		? info.title
		: (info.title + u" – "_q + info.subtitle);

	BNotification notification(B_INFORMATION_NOTIFICATION);
	notification.SetGroup("Telegram");
	notification.SetTitle(title.toUtf8().constData());
	notification.SetContent(info.message.toUtf8().constData());
	notification.SetMessageID(MessageId(context).constData());
	notification.SetOnClickApp(_signature.constData());
	const auto argument = QByteArray(AppMessages::kNotificationArgument)
		+ MessageId(context)
		+ ':'
		+ QByteArray::number(info.itemId.bare);
	notification.AddOnClickArg(argument.constData());

	if (!info.options.hideNameAndPhoto) {
		const auto userpic = Window::Notifications::GenerateUserpic(
			peer,
			userpicView);
		if (const auto icon = MakeIcon(userpic)) {
			notification.SetIcon(icon);
			delete icon;
		}
	}
	notification.Send();
}

Manager::Manager(not_null<Window::Notifications::System*> system)
: NativeManager(system)
, _private(std::make_unique<Private>(this)) {
}

Manager::~Manager() = default;

void Manager::doShowNativeNotification(
		NotificationInfo &&info,
		Ui::PeerUserpicView &userpicView) {
	_private->showNotification(std::move(info), userpicView);
}

void Manager::doClearAllFast() {
}

void Manager::doClearFromItem(not_null<HistoryItem*> item) {
}

void Manager::doClearFromTopic(not_null<Data::ForumTopic*> topic) {
}

void Manager::doClearFromSublist(not_null<Data::SavedSublist*> sublist) {
}

void Manager::doClearFromHistory(not_null<History*> history) {
}

void Manager::doClearFromSession(not_null<Main::Session*> session) {
}

bool Manager::doSkipToast() const {
	return false;
}

void Manager::doMaybePlaySound(Fn<void()> playSound) {
	playSound();
}

void Manager::doMaybeFlashBounce(Fn<void()> flashBounce) {
	flashBounce();
}

} // namespace Notifications
} // namespace Platform
