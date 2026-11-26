#!/bin/bash
# Run before OpenWrt's rstrip: retain target paths needed by dlopen plugins.
set -euo pipefail
patchelf=$1
readelf=$2
staging=$3
root=$4
[[ -d $root && -x $patchelf ]]
command -v "$readelf" >/dev/null
while IFS= read -r -d '' file; do
    dynamic=$("$readelf" -d "$file" 2>/dev/null) || continue
    [[ $dynamic == *'(RPATH)'* || $dynamic == *'(RUNPATH)'* ]] || continue
    old=$("$patchelf" --print-rpath "$file")
    remaining=$old
    new=
    separator=
    while :; do
        entry=${remaining%%:*}
        case "$entry" in "$staging"/*) entry=${entry#"$staging"} ;; esac
        new+="$separator$entry"
        separator=:
        case "$remaining" in *:*) remaining=${remaining#*:} ;; *) break ;; esac
    done
    [[ $new != "$old" ]] || continue
    mode=$(stat -c '%a' "$file")
    flags=()
    [[ $dynamic != *'(RPATH)'* ]] || flags+=(--force-rpath)
    "$patchelf" "${flags[@]}" --set-rpath "$new" "$file"
    chmod "$mode" "$file"
done < <(find "$root" -type f -print0)
