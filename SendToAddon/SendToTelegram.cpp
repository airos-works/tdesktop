#include <limits.h>

#include <StorageKit.h>
#include <SupportKit.h>
#include <AppKit.h>

#include <add-ons/tracker/TrackerAddOn.h>

const char* kTelegramSignature = "application/x-vnd.Telegram";

void
process_refs(entry_ref base_ref, BMessage* message, void* reserved)
{
	char **argv = new char*[ARG_MAX];

	int argc = 0;
	argv[argc++] = strdup("-sendpath");
	argv[argc] = NULL;

	BEntry entry;	
	entry_ref ref;

	int32 i;

	for (i = 0; message->FindRef("refs", i, &ref) == B_OK; i++) {
		if (entry.SetTo(&ref, true) != B_OK)
			continue;
		if (entry.IsDirectory())
			continue;

		BPath path;
		entry.GetPath(&path);
		argv[argc++] = strdup(path.Path());
		argv[argc] = NULL;
	}

	if (argc > 1)
		be_roster->Launch(kTelegramSignature, argc, argv);

	for(i = 0; argv[i] != NULL; i++)
		free(argv[i]);

	delete[] argv;
}
