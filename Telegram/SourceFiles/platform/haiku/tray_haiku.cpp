/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "platform/haiku/tray_haiku.h"

#include "base/timer.h"
#include "base/qt_signal_producer.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "core/launcher.h"
#include "core/version.h"
#include "platform/haiku/app_messages_haiku.h"
#include "platform/haiku/deskbar_protocol_haiku.h"
#include "window/main_window.h"
#include "styles/style_window.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtGui/QGuiApplication>

#include "base/platform/haiku/base_haiku_begin.h"
#include <Application.h>
#include <Deskbar.h>
#include <Entry.h>
#include <Message.h>
#include <Messenger.h>
#include <Roster.h>
#include "base/platform/haiku/base_haiku_end.h"

namespace Platform {
namespace {

using namespace DeskbarTray;

constexpr auto kDeskbarSignature = "application/x-vnd.Be-TSKB";
constexpr auto kAddOnName = "Telegram Deskbar item";
constexpr auto kReAddDelay = crl::time(1000);
constexpr int kIconSizes[] = { 16, 22, 32, 48, 64 };

// Next to the program (a build), or in the package's add-ons/Telegram.
[[nodiscard]] QString AddOnPath() {
	const auto exe = cExeDir();
	for (const auto &path : {
			exe + kAddOnName,
			exe + u"../add-ons/Telegram/"_q + kAddOnName }) {
		if (QFile::exists(path)) {
			return QFileInfo(path).canonicalFilePath();
		}
	}
	return QString();
}

} // namespace

struct Tray::Private {
	struct Action {
		uint32 id = 0;
		QString text;
		Fn<void()> callback;
	};

	BMessenger item;
	bool wanted = false;
	bool added = false;
	int counter = -1;
	bool muted = false;
	std::vector<Action> actions;
	uint32 nextActionId = 1;
	base::flat_map<int, QImage> logos;
	base::Timer reAdd;
};

Tray::Tray() : _private(std::make_unique<Private>()) {
	_private->reAdd.setCallback([=] {
		if (_private->wanted && !_private->added) {
			createIcon();
		}
	});
	AppMessages::Start();
	AppMessages::SetTrayHandler([=](not_null<BMessage*> message) {
		switch (message->what) {
		case kHello: {
			auto item = BMessenger();
			if (message->FindMessenger("item", &item) != B_OK) {
				return;
			}
			_private->item = item;
			BMessage reply(kHello);
			reply.AddMessenger("app", BMessenger(be_app));
			_private->item.SendMessage(&reply);
			BMessage tooltip(kTooltip);
			tooltip.AddString("text", QString(AppName.utf16()).toUtf8().constData());
			_private->item.SendMessage(&tooltip);
			_private->counter = -1;
			sendIcon();
			sendMenu();
		} break;
		case kActivated:
			_iconClicks.fire({});
			break;
		case kMenuOpening:
			_aboutToShowRequests.fire({});
			break;
		case kMenuItem: {
			auto id = uint32(0);
			if (message->FindUInt32("id", &id) != B_OK) {
				return;
			}
			for (const auto &action : _private->actions) {
				if (action.id == id && action.callback) {
					// The callback may change the menu (and this list).
					const auto callback = action.callback;
					callback();
					break;
				}
			}
		} break;
		case kDetached:
			_private->item = BMessenger();
			_private->added = false;
			if (_private->wanted) {
				// Deskbar restarted, or the package was updated: the item
				// comes back once Deskbar is there again.
				_private->reAdd.callOnce(kReAddDelay);
			}
			break;
		}
	});

	// The menu's "Show/Hide Telegram" follows the windows: the item shows
	// its menu without waiting for Telegram.
	base::qt_signal_producer(
		qApp,
		&QGuiApplication::applicationStateChanged
	) | rpl::on_next([=] {
		_aboutToShowRequests.fire({});
	}, _lifetime);
}

Tray::~Tray() {
	AppMessages::SetTrayHandler(nullptr);
	destroyIcon();
}

void Tray::createIcon() {
	_private->wanted = true;
	if (_private->added || !be_roster->IsRunning(kDeskbarSignature)) {
		return;
	}
	const auto path = AddOnPath();
	if (path.isEmpty()) {
		LOG(("Tray Error: no '%1' next to Telegram.").arg(kAddOnName));
		return;
	}
	auto ref = entry_ref();
	if (BEntry(QFile::encodeName(path).constData()).GetRef(&ref) != B_OK) {
		return;
	}
	auto deskbar = BDeskbar();
	if (deskbar.HasItem(kItemName)) {
		// Left by a Telegram that did not quit (or another build).
		deskbar.RemoveItem(kItemName);
	}
	const auto result = deskbar.AddItem(&ref);
	_private->added = (result == B_OK);
	LOG(("System tray available: %1").arg(Logs::b(_private->added)));
}

void Tray::destroyIcon() {
	_private->wanted = false;
	if (_private->item.IsValid()) {
		BMessage goodbye(kGoodbye);
		_private->item.SendMessage(&goodbye);
		_private->item = BMessenger();
	}
	if (_private->added) {
		_private->added = false;
		BDeskbar().RemoveItem(kItemName);
	}
}

void Tray::updateIcon() {
	sendIcon();
}

void Tray::sendIcon() {
	if (!_private->item.IsValid()) {
		return;
	}
	const auto counter = Core::App().unreadBadge();
	const auto muted = Core::App().unreadBadgeMuted();
	if (counter == _private->counter && muted == _private->muted) {
		return;
	}
	_private->counter = counter;
	_private->muted = muted;

	BMessage icon(kIcon);
	for (const auto size : kIconSizes) {
		auto &logo = _private->logos[size];
		if (logo.isNull()) {
			logo = Window::Logo().scaled(
				size,
				size,
				Qt::IgnoreAspectRatio,
				Qt::SmoothTransformation);
		}
		auto image = (counter > 0)
			? Window::WithSmallCounter(logo, {
				.size = size,
				.count = counter,
				.bg = muted ? st::trayCounterBgMute : st::trayCounterBg,
				.fg = st::trayCounterFg,
			})
			: logo;
		// B_RGBA32: straight alpha, BGRA in memory, as Format_ARGB32.
		image = image.convertToFormat(QImage::Format_ARGB32);
		image.setDevicePixelRatio(1.);
		auto pixels = QByteArray();
		pixels.reserve(image.width() * image.height() * 4);
		for (auto y = 0; y != image.height(); ++y) {
			pixels.append(
				reinterpret_cast<const char*>(image.constScanLine(y)),
				image.width() * 4);
		}
		icon.AddInt32("width", image.width());
		icon.AddInt32("height", image.height());
		icon.AddData(
			"pixels",
			B_RAW_TYPE,
			pixels.constData(),
			pixels.size(),
			false);
	}
	_private->item.SendMessage(&icon);
}

void Tray::createMenu() {
}

void Tray::destroyMenu() {
	_actionsLifetime.destroy();
	_private->actions.clear();
	sendMenu();
}

void Tray::addAction(rpl::producer<QString> text, Fn<void()> &&callback) {
	const auto id = _private->nextActionId++;
	_private->actions.push_back({
		.id = id,
		.callback = std::move(callback),
	});
	std::move(
		text
	) | rpl::on_next([=](const QString &text) {
		for (auto &action : _private->actions) {
			if (action.id == id) {
				action.text = text;
				sendMenu();
				break;
			}
		}
	}, _actionsLifetime);
}

void Tray::sendMenu() {
	if (!_private->item.IsValid()) {
		return;
	}
	BMessage menu(kMenu);
	for (const auto &action : _private->actions) {
		if (action.text.isEmpty()) {
			continue;
		}
		menu.AddString("label", action.text.toUtf8().constData());
		menu.AddUInt32("id", action.id);
	}
	_private->item.SendMessage(&menu);
}

void Tray::showTrayMessage() const {
}

bool Tray::hasTrayMessageSupport() const {
	return false;
}

rpl::producer<> Tray::aboutToShowRequests() const {
	return _aboutToShowRequests.events();
}

rpl::producer<> Tray::showFromTrayRequests() const {
	return rpl::never<>();
}

rpl::producer<> Tray::hideToTrayRequests() const {
	return rpl::never<>();
}

rpl::producer<> Tray::iconClicks() const {
	return _iconClicks.events();
}

bool Tray::hasIcon() const {
	return _private->wanted;
}

rpl::lifetime &Tray::lifetime() {
	return _lifetime;
}

bool HasMonochromeSetting() {
	return false;
}

} // namespace Platform
