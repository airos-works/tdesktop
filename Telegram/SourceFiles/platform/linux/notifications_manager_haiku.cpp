/* 
This file is part of Telegram Desktop for Haiku,

For license and copyright information please follow this link:
https://github.com/desktop-app/legal/blob/master/LEGAL

Copyright (c) 2018-2021 Gerasim Troeglazov, 3dEyes@gmail.com
*/


#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "platform/linux/notifications_manager_linux.h"

#include "base/options.h"
#include "base/platform/base_platform_info.h"
#include "core/application.h"
#include "core/sandbox.h"
#include "core/core_settings.h"
#include "data/data_forum_topic.h"
#include "history/history.h"
#include "history/history_item.h"
#include "ui/empty_userpic.h"
#include "main/main_session.h"
#include "lang/lang_keys.h"
#include "base/weak_ptr.h"
#include "window/notifications_utilities.h"
#include "styles/style_window.h"

#define USE_HAIKU_UINT64
#include "base/haiku_types.h"

#include <AppKit.h>
#include <SupportKit.h>
#include <Notification.h>
#include <InterfaceKit.h>
#include <TranslationUtils.h>
#include <OS.h>

#define NOTIFY_MESSAGE_DEEP	16
#define NOTIFY_PORT_NAME "tg_notify"
#define NOTIFY_GATE_NAME "/bin/qnotify"
#define NOTIFY_MESSAGE 'QNTF'

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

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
			auto manager = std::make_unique<Manager>(system);
			return manager;
		});
		return;
	}
	system->setManager(nullptr);
}

class Manager::Private : public QObject {

public:
	explicit Private(not_null<Manager*> manager);

	void showNotification(
		NotificationInfo &&info,
		Ui::PeerUserpicView &userpicView);

	~Private();

private:
	void CheckNotificationAction();

	QTimer *_checkNotificationTimer;
	const not_null<Manager*> _manager;
	int32 _portId;
};

Manager::Private::Private(not_null<Manager*> manager)
: _manager(manager) {
	_portId = create_port(NOTIFY_MESSAGE_DEEP, NOTIFY_PORT_NAME);
	_checkNotificationTimer = new QTimer(this);
	connect(_checkNotificationTimer, &QTimer::timeout, this, &Manager::Private::CheckNotificationAction);
	_checkNotificationTimer->start(250);
}

Manager::Private::~Private() {
	delete _checkNotificationTimer;
	if (_portId > 0)
		delete_port(_portId);
}

void Manager::Private::CheckNotificationAction()
{
	ssize_t msgSize;

	do {
		msgSize = port_buffer_size_etc(_portId, B_RELATIVE_TIMEOUT, 0);
	} while (msgSize == B_INTERRUPTED);

	if (msgSize < 0)
		return;

	char* buffer = new(std::nothrow) char[msgSize];
	if (!buffer)
		return;

	int32 code;
	status_t result;
	do {
		result = read_port(_portId, &code, buffer, msgSize);
	} while (result == B_INTERRUPTED);

	BMessage message;

	if (result < 0 || code != NOTIFY_MESSAGE) {
		delete[] buffer;
		return;
	}
	
	if (message.Unflatten(buffer) == B_OK) {
		uint64 sessionId = message.GetUInt64("session", 0);
		uint64 peerId = message.GetUInt64("peer", 0);
		int64 messageId = message.GetInt64("message", 0);

		const auto key = ContextId{
			.sessionId = sessionId,
			.peerId = PeerId(peerId),
			.topicRootId = 0,
		};
		const auto notificationId = NotificationId{ .contextId = key, .msgId = messageId };

		crl::on_main(this, [=] {
			_manager->notificationActivated(notificationId);
		});
	}
	delete[] buffer;
}

void Manager::Private::showNotification(
		NotificationInfo &&info,
		Ui::PeerUserpicView &userpicView) {
	const auto peer = info.peer;
	const auto options = info.options;
	const auto titleText = info.title;
	const auto subtitleText = info.subtitle;
	const auto msgText = info.message;
	const auto msgId = info.itemId;

	QString args = QString(
		"mode:port "
		"target:tg_notify "
		"session:uint64:%1 "
		"peer:uint64:%2 "
		"message:int64:%3")
		.arg(peer->session().uniqueId()).arg(peer->id.value).arg(msgId.bare);

	BNotification notify(B_INFORMATION_NOTIFICATION);
	notify.SetGroup("Telegram");
	notify.SetTitle(titleText.toUtf8().data());
	notify.SetContent(msgText.toUtf8().data());
	entry_ref ref;
	BEntry entry(NOTIFY_GATE_NAME);
	entry.GetRef(&ref);
	notify.SetOnClickFile(&ref);
	notify.AddOnClickArg(BString(args.toUtf8().data()));

	if (!options.hideNameAndPhoto) {
		QImage userpic = Window::Notifications::GenerateUserpic(peer, userpicView);
		BBitmap *icon = new BBitmap(BRect(0, 0, userpic.width() - 1, userpic.height() - 1), B_RGBA32);
		icon->SetBits((void*)userpic.bits(), userpic.sizeInBytes(), 0, B_RGBA32);
		notify.SetIcon(icon);
		delete icon;
	}

	notify.Send();
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

void Manager::doClearAllFast() { }

void Manager::doClearFromItem(not_null<HistoryItem*> item) { }

void Manager::doClearFromHistory(not_null<History*> history) { }

void Manager::doClearFromSession(not_null<Main::Session*> session) { }

void Manager::doClearFromTopic(not_null<Data::ForumTopic*> topic) { }

void Manager::doClearFromSublist(not_null<Data::SavedSublist*> sublist) { }

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
