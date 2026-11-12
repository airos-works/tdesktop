#!/usr/bin/env bash
# One-time: turn the desktop-app submodules (and the third-party ones air/OS
# changes) into squashed git subtrees, so the fork is one repository that
# builds as checked out. airos/sync-upstream.sh pulls them forward later.
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"
FLATTEN=(cmake Telegram/codegen Telegram/lib_base Telegram/lib_crl
	Telegram/lib_lottie Telegram/lib_qr Telegram/lib_rpl Telegram/lib_spellcheck
	Telegram/lib_storage Telegram/lib_tl Telegram/lib_translate Telegram/lib_ui
	Telegram/lib_webrtc Telegram/lib_webview Telegram/ThirdParty/tgcalls
	Telegram/ThirdParty/libfido2)
for p in "${FLATTEN[@]}"; do
	url=$(git config -f .gitmodules --get "submodule.$p.url")
	commit=$(git ls-tree HEAD "$p" | awk '{print $3}')
	moddir=$(git -C "$p" rev-parse --absolute-git-dir)
	echo "== $p $commit ($url)"
	printf '%s %s %s\n' "$p" "$url" "$commit" >> airos/subtrees.txt
	git submodule deinit -q -f "$p"
	git rm -q --cached "$p"
	git config -f .gitmodules --remove-section "submodule.$p"
	rm -rf "$p"
	git add .gitmodules
	git commit -q -m "airos: drop the $p submodule (becomes a subtree)"
	git fetch -q "$moddir" "$commit"
	git subtree add -q --prefix="$p" --squash FETCH_HEAD \
		-m "airos: $p from $url at $commit"
done
git add airos/subtrees.txt
git commit -q -m "airos: record the upstream commits of the subtrees"
