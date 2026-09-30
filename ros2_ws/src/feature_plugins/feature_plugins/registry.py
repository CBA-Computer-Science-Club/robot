"""Resolve only explicitly approved installed entry points (not a sandbox)."""
import re

_APPROVAL = re.compile(r"([A-Za-z0-9_-]+):([A-Za-z0-9_-]+)=([A-Za-z_][\w.]*:[A-Za-z_]\w*)\Z")


def resolve_plugins(allowlist, entry_points):
    """Approval format: distribution:entry_name=module:object."""
    allowed = set(allowlist)
    if len(allowed) != len(allowlist) or any(not _APPROVAL.fullmatch(x) for x in allowed):
        raise ValueError("Invalid or duplicate plugin approval")
    entries = {}
    for ep in entry_points:
        if ep.group != "robot.feature_plugins":
            continue
        key = f"{ep.dist.name}:{ep.name}={ep.value}"
        if key in allowed:
            entries.setdefault(key, []).append(ep)
    if any(len(entries.get(key, [])) != 1 for key in allowed):
        raise ValueError("Approved plugin missing or ambiguous; no plugins loaded")
    return [(entries[key][0].name, entries[key][0].load()) for key in sorted(allowed)]


def start_plugins(allowlist, entry_points, api_factory):
    plugins = []
    for name, plugin_class in resolve_plugins(allowlist, entry_points):
        plugin = plugin_class()
        plugin.start(api_factory(name))
        plugins.append(plugin)
    return plugins
