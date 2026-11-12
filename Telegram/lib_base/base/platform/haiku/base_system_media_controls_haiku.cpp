// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/base_platform_system_media_controls.h"

// air/OS has no system media session (MPRIS-like) service: the controls
// report themselves unsupported and Telegram keeps its own player bar.

namespace base::Platform {

struct SystemMediaControls::Private {
};

SystemMediaControls::SystemMediaControls() = default;

SystemMediaControls::~SystemMediaControls() = default;

bool SystemMediaControls::init() {
	return false;
}

void SystemMediaControls::setApplicationName(const QString &name) {
}

void SystemMediaControls::setEnabled(bool enabled) {
}

void SystemMediaControls::setIsNextEnabled(bool value) {
}

void SystemMediaControls::setIsPreviousEnabled(bool value) {
}

void SystemMediaControls::setIsPlayPauseEnabled(bool value) {
}

void SystemMediaControls::setIsStopEnabled(bool value) {
}

void SystemMediaControls::setPlaybackStatus(PlaybackStatus status) {
}

void SystemMediaControls::setLoopStatus(LoopStatus status) {
}

void SystemMediaControls::setShuffle(bool value) {
}

void SystemMediaControls::setTitle(const QString &title) {
}

void SystemMediaControls::setArtist(const QString &artist) {
}

void SystemMediaControls::setThumbnail(const QImage &thumbnail) {
}

void SystemMediaControls::setDuration(int duration) {
}

void SystemMediaControls::setPosition(int position) {
}

void SystemMediaControls::setPlaybackRate(float64 rate) {
}

void SystemMediaControls::setVolume(float64 volume) {
}

void SystemMediaControls::clearThumbnail() {
}

void SystemMediaControls::clearMetadata() {
}

void SystemMediaControls::updateDisplay() {
}

auto SystemMediaControls::commandRequests() const
-> rpl::producer<SystemMediaControls::Command> {
	return rpl::never<SystemMediaControls::Command>();
}

rpl::producer<float64> SystemMediaControls::seekRequests() const {
	return rpl::never<float64>();
}

rpl::producer<float64> SystemMediaControls::volumeChangeRequests() const {
	return rpl::never<float64>();
}

rpl::producer<> SystemMediaControls::updatePositionRequests() const {
	return rpl::never<>();
}

bool SystemMediaControls::seekingSupported() const {
	return false;
}

bool SystemMediaControls::volumeSupported() const {
	return false;
}

bool SystemMediaControls::Supported() {
	return false;
}

} // namespace base::Platform
