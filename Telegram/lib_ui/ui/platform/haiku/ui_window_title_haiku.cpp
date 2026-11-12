// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "ui/platform/haiku/ui_window_title_haiku.h"

namespace Ui {
namespace Platform {
namespace {

// With Telegram's own title bar: close on the left, zoom on the right,
// where app_server's decorator has them.
class TitleControlsLayoutImpl : public TitleControlsLayout {
public:
	TitleControlsLayoutImpl() : TitleControlsLayout(TitleControls::Layout{
		.left = { TitleControls::Control::Close },
		.right = {
			TitleControls::Control::Minimize,
			TitleControls::Control::Maximize,
		},
	}) {
	}

};

} // namespace

std::shared_ptr<TitleControlsLayout> TitleControlsLayout::Create() {
	return std::make_shared<TitleControlsLayoutImpl>();
}

} // namespace Platform
} // namespace Ui
