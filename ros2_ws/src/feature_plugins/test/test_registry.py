import unittest
from feature_plugins.registry import resolve_plugins, start_plugins


class RegistryTests(unittest.TestCase):
    def test_empty_allowlist_loads_nothing(self):
        self.assertEqual(resolve_plugins([], []), [])

    def test_only_exact_approved_distribution_name_and_target_load(self):
        class Entry:
            group = "robot.feature_plugins"
            name = "pulse"
            value = "feature_plugins.example:HeartbeatPlugin"
            dist = type("Dist", (), {"name": "feature_plugins"})()
            calls = 0

            def load(self):
                self.calls += 1
                return object

        entry = Entry()
        approved = "feature_plugins:pulse=feature_plugins.example:HeartbeatPlugin"
        self.assertEqual(resolve_plugins([], [entry]), [])
        self.assertEqual(entry.calls, 0)
        self.assertEqual(resolve_plugins([approved], [entry]), [("pulse", object)])
        self.assertEqual(entry.calls, 1)

    def test_missing_approval_fails_closed_before_any_plugin_loads(self):
        class Entry:
            group = "robot.feature_plugins"
            name = "pulse"
            value = "feature_plugins.example:HeartbeatPlugin"
            dist = type("Dist", (), {"name": "feature_plugins"})()
            def load(self):
                raise AssertionError("Should not load")
        with self.assertRaises(ValueError):
            resolve_plugins(["feature_plugins:missing=feature_plugins.example:HeartbeatPlugin"], [Entry()])

    def test_approved_plugin_starts_with_scoped_api(self):
        class Plugin:
            def start(self, api):
                self.api = api
        class Entry:
            group = "robot.feature_plugins"
            name = "pulse"
            value = "feature_plugins.example:HeartbeatPlugin"
            dist = type("Dist", (), {"name": "feature_plugins"})()
            def load(self):
                return Plugin
        instances = start_plugins(["feature_plugins:pulse=feature_plugins.example:HeartbeatPlugin"], [Entry()], lambda name: name)
        self.assertEqual(instances[0].api, "pulse")


if __name__ == "__main__":
    unittest.main()
