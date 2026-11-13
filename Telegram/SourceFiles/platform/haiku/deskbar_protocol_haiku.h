/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

// Between Telegram (platform/haiku/tray_haiku.cpp) and its Deskbar item
// (platform/haiku/deskbar_item_haiku.cpp, an add-on loaded into Deskbar's
// own team: it links libbe alone, never Telegram or Qt).

namespace Platform::DeskbarTray {

// The Deskbar item's (and its view's) name.
inline constexpr auto kItemName = "Telegram";

// Telegram's signatures: the package's, and test builds' (their own, so
// that they run beside an installed Telegram).
inline constexpr const char *kSignatures[] = {
	"application/x-vnd.telegram",
	"application/x-vnd.telegram-airos-test",
};

enum : unsigned int {
	// item -> Telegram: "item" (its messenger); Telegram -> item: "app"
	kHello = 'tgTH',
	// Telegram -> item: "width", "height" (int32) and "pixels" (B_RAW_TYPE,
	// straight-alpha B_RGBA32 rows), one of each per size
	kIcon = 'tgTI',
	// Telegram -> item: "text"
	kTooltip = 'tgTT',
	// Telegram -> item: "label" (string) and "id" (uint32) per entry
	kMenu = 'tgTM',
	// Telegram -> item: Telegram's tray is going
	kGoodbye = 'tgTB',
	// item -> Telegram: a primary click
	kActivated = 'tgTA',
	// item -> Telegram: before the menu opens (Telegram refreshes it)
	kMenuOpening = 'tgTO',
	// item -> Telegram: a menu choice, "id" (uint32)
	kMenuItem = 'tgTC',
	// item -> Telegram: Deskbar took the item out (it quit, or the
	// add-on's file went: a package update)
	kDetached = 'tgTD',
};

[[nodiscard]] inline bool IsTrayMessage(unsigned int what) {
	return (what == kHello)
		|| (what == kActivated)
		|| (what == kMenuOpening)
		|| (what == kMenuItem)
		|| (what == kDetached);
}

} // namespace Platform::DeskbarTray
