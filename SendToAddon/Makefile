NAME = Send\ to\ Telegram
TYPE = SHARED
APP_MIME_SIG = application/x-vnd.Telegram-SendTo
SRCS = SendToTelegram.cpp
LIBS = be
OPTIMIZE := FULL
WARNINGS = NONE
RDEFS = resource.rdef

DEVEL_DIRECTORY := \
	$(shell findpaths -r "makefile_engine" B_FIND_PATH_DEVELOP_DIRECTORY)
include $(DEVEL_DIRECTORY)/etc/makefile-engine
