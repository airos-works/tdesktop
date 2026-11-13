/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
// Telegram's tray icon on air/OS: a Deskbar add-on ("Telegram Deskbar
// item", add-ons/Telegram), loaded into Deskbar's team. Telegram adds it
// with BDeskbar::AddItem() and takes it out when its tray goes. The view
// says hello to Telegram with its messenger; Telegram sends the icon (with
// the unread badge) in a few sizes, the tooltip and the menu, and gets the
// clicks back. Deskbar brings its items back after a restart: an item that
// finds no Telegram running takes itself out again.
//
// It replaces Qt's QSystemTrayIcon, whose Haiku plugin forks a helper
// program (qsystray) to put a generic replicant in Deskbar.
#include "platform/haiku/deskbar_protocol_haiku.h"

#include <Bitmap.h>
#include <Deskbar.h>
#include <MenuItem.h>
#include <Message.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <View.h>
#include <Window.h>

#include <cstring>
#include <dlfcn.h>

using namespace Platform::DeskbarTray;

namespace {

// Device pixels per logical one (air/OS's display render scale).
float DeviceScale() {
	using GetLayout = status_t (*)(BMessage &);
	static const auto getLayout = reinterpret_cast<GetLayout>(dlsym(
		RTLD_DEFAULT,
		"_ZN8BPrivate18get_display_layoutER8BMessage"));
	BMessage layout;
	if (!getLayout || getLayout(layout) != B_OK) {
		return 1.f;
	}
	int32 best = 0;
	BMessage display;
	for (int32 i = 0; layout.FindMessage("display", i, &display) == B_OK; ++i) {
		int32 scale = 0;
		if (display.GetBool("enabled", true)
			&& display.FindInt32("render scale", &scale) == B_OK
			&& scale > best) {
			best = scale;
		}
	}
	return (best >= 100 && best <= 400) ? (best / 100.f) : 1.f;
}

BMessenger FindTelegram() {
	for (const auto signature : kSignatures) {
		if (be_roster->IsRunning(signature)) {
			return BMessenger(signature);
		}
	}
	return BMessenger();
}

// Deskbar must not be asked to remove an item from its own window thread.
int32 RemoveLater(void *) {
	snooze(200000);
	BDeskbar().RemoveItem(kItemName);
	return 0;
}

} // namespace

// The class has a name of this build's own (TELEGRAM_TRAY_VIEW, set by
// CMake): Deskbar's shelf instantiates an item from its archive by class
// name, from the first of the images it loaded that has that name, and it
// never unloads one. A Telegram updated while Deskbar runs gets its own.
#ifndef TELEGRAM_TRAY_VIEW
#define TELEGRAM_TRAY_VIEW TelegramTrayView
#endif // TELEGRAM_TRAY_VIEW
#define TELEGRAM_STRINGIFY_(x) #x
#define TELEGRAM_STRINGIFY(x) TELEGRAM_STRINGIFY_(x)

class _EXPORT TELEGRAM_TRAY_VIEW final : public BView {
public:
	explicit TELEGRAM_TRAY_VIEW(BRect frame)
	: BView(frame, kItemName, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW) {
	}

	explicit TELEGRAM_TRAY_VIEW(BMessage *archive) : BView(archive) {
	}

	~TELEGRAM_TRAY_VIEW() override {
		delete _bitmap;
	}

	// The archive names no "add_on" signature: the shelf would load
	// Telegram itself into Deskbar should a lookup by name ever fail.
	static BArchivable *Instantiate(BMessage *archive);

	void AttachedToWindow() override {
		BView::AttachedToWindow();
		AdoptParentColors();
		SetDrawingMode(B_OP_ALPHA);
		SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		hello();
	}

	// Deskbar unloads the item when it quits and when the add-on's file
	// goes (a package update while Telegram runs): Telegram puts it back.
	void DetachedFromWindow() override {
		if (_telegram.IsValid()) {
			BMessage gone(kDetached);
			_telegram.SendMessage(&gone);
		}
		BView::DetachedFromWindow();
	}

	void Draw(BRect) override {
		if (_bitmap) {
			DrawBitmap(
				_bitmap,
				_bitmap->Bounds(),
				Bounds(),
				B_FILTER_BITMAP_BILINEAR);
		}
	}

	void MouseDown(BPoint where) override {
		const auto current = Window()->CurrentMessage();
		auto buttons = int32(B_PRIMARY_MOUSE_BUTTON);
		if (current) {
			current->FindInt32("buttons", &buttons);
		}
		if (!_telegram.IsValid()) {
			hello();
			return;
		}
		if (buttons & B_SECONDARY_MOUSE_BUTTON) {
			BMessage opening(kMenuOpening);
			_telegram.SendMessage(&opening);
			showMenu(ConvertToScreen(where));
			return;
		}
		BMessage clicked(kActivated);
		_telegram.SendMessage(&clicked);
	}

	void MessageReceived(BMessage *message) override {
		switch (message->what) {
		case kHello: {
			BMessenger to;
			if (message->FindMessenger("app", &to) == B_OK) {
				_telegram = to;
			}
		} break;
		case kIcon:
			setIcon(message);
			break;
		case kTooltip: {
			const char *text = nullptr;
			SetToolTip((message->FindString("text", &text) == B_OK)
				? text
				: nullptr);
		} break;
		case kMenu:
			_menu = *message;
			break;
		case kGoodbye:
			_telegram = BMessenger();
			break;
		default:
			BView::MessageReceived(message);
		}
	}

private:
	void hello() {
		const auto telegram = FindTelegram();
		if (!telegram.IsValid()) {
			// Deskbar brought the item back after a restart and Telegram
			// isn't running: out again.
			resume_thread(spawn_thread(
				RemoveLater,
				"Telegram tray removal",
				B_LOW_PRIORITY,
				nullptr));
			return;
		}
		BMessage hi(kHello);
		hi.AddMessenger("item", BMessenger(this));
		telegram.SendMessage(&hi);
	}

	void setIcon(BMessage *message) {
		// The smallest size at least as big as the slot in device pixels.
		const auto want = (Bounds().Height() + 1) * DeviceScale();
		auto best = int32(-1);
		auto bestWidth = int32(0);
		auto width = int32(0);
		for (int32 i = 0
			; message->FindInt32("width", i, &width) == B_OK
			; ++i) {
			if (best < 0
				|| (bestWidth < want && width > bestWidth)
				|| (width >= want && width < bestWidth)) {
				best = i;
				bestWidth = width;
			}
		}
		auto height = int32(0);
		if (best < 0
			|| message->FindInt32("height", best, &height) != B_OK) {
			return;
		}
		const void *data = nullptr;
		auto size = ssize_t(0);
		if (message->FindData("pixels", B_RAW_TYPE, best, &data, &size) != B_OK
			|| size != ssize_t(bestWidth) * height * 4) {
			return;
		}
		const auto bitmap = new BBitmap(
			BRect(0, 0, bestWidth - 1, height - 1),
			B_RGBA32);
		if (bitmap->InitCheck() != B_OK) {
			delete bitmap;
			return;
		}
		for (int32 y = 0; y < height; ++y) {
			memcpy(
				static_cast<uint8*>(bitmap->Bits())
					+ size_t(y) * bitmap->BytesPerRow(),
				static_cast<const uint8*>(data) + size_t(y) * bestWidth * 4,
				size_t(bestWidth) * 4);
		}
		delete _bitmap;
		_bitmap = bitmap;
		Invalidate();
	}

	void showMenu(BPoint where) {
		const auto menu = new BPopUpMenu("Telegram", false, false);
		menu->SetAsyncAutoDestruct(true);
		const char *label = nullptr;
		auto id = uint32(0);
		for (int32 i = 0
			; _menu.FindString("label", i, &label) == B_OK
			&& _menu.FindUInt32("id", i, &id) == B_OK
			; ++i) {
			const auto chosen = new BMessage(kMenuItem);
			chosen->AddUInt32("id", id);
			const auto item = new BMenuItem(label, chosen);
			item->SetTarget(_telegram);
			menu->AddItem(item);
		}
		if (!menu->CountItems()) {
			delete menu;
			return;
		}
		menu->Go(
			where,
			true,
			true,
			BRect(where - BPoint(4, 4), where + BPoint(4, 4)),
			true);
	}

	BBitmap *_bitmap = nullptr;
	BMessenger _telegram;
	BMessage _menu;

};

BArchivable *TELEGRAM_TRAY_VIEW::Instantiate(BMessage *archive) {
	return validate_instantiation(archive, TELEGRAM_STRINGIFY(TELEGRAM_TRAY_VIEW))
		? new TELEGRAM_TRAY_VIEW(archive)
		: nullptr;
}

extern "C" _EXPORT BView *instantiate_deskbar_item(
		float maxWidth,
		float maxHeight) {
	return new TELEGRAM_TRAY_VIEW(BRect(0, 0, maxHeight - 1, maxHeight - 1));
}
