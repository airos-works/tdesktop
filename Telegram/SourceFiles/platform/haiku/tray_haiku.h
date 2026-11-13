/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "platform/platform_tray.h"

namespace Platform {

// air/OS: the tray icon is Telegram's Deskbar item (deskbar_item_haiku.cpp,
// an add-on Deskbar loads), with the unread badge and a native menu.
class Tray final {
public:
	Tray();
	~Tray();

	[[nodiscard]] rpl::producer<> aboutToShowRequests() const;
	[[nodiscard]] rpl::producer<> showFromTrayRequests() const;
	[[nodiscard]] rpl::producer<> hideToTrayRequests() const;
	[[nodiscard]] rpl::producer<> iconClicks() const;

	[[nodiscard]] bool hasIcon() const;

	void createIcon();
	void destroyIcon();

	void updateIcon();

	void createMenu();
	void destroyMenu();

	void addAction(rpl::producer<QString> text, Fn<void()> &&callback);

	void showTrayMessage() const;
	[[nodiscard]] bool hasTrayMessageSupport() const;

	[[nodiscard]] rpl::lifetime &lifetime();

private:
	struct Private;

	void sendIcon();
	void sendMenu();

	const std::unique_ptr<Private> _private;

	rpl::event_stream<> _iconClicks;
	rpl::event_stream<> _aboutToShowRequests;

	rpl::lifetime _actionsLifetime;
	rpl::lifetime _lifetime;

};

} // namespace Platform
