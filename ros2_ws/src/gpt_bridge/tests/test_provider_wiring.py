"""Static wiring checks supplement standalone provider behavior tests; not ROS integration."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class ProviderWiring(unittest.TestCase):
    def test_credentials_are_environment_only(self):
        code = (ROOT / 'src/gpt_bridge.cpp').read_text()
        self.assertIn('std::getenv("ANTHROPIC_API_KEY")', code)
        self.assertIn('std::getenv("OPENAI_API_KEY")', code)
        self.assertNotIn('declare_parameter<std::string>("api_key"', code)
        self.assertIn('gpt_bridge::provider_headers', code)

    def test_transport_enforces_tls_and_no_redirects(self):
        code = (ROOT / 'src/gpt_bridge.cpp').read_text()
        self.assertIn('CURLPROTO_HTTPS', code)
        self.assertIn('CURLOPT_SSL_VERIFYPEER, 1L', code)
        self.assertIn('CURLOPT_SSL_VERIFYHOST, 2L', code)
        self.assertIn('CURLOPT_FOLLOWLOCATION, 0L', code)
        self.assertIn('gpt_bridge::provider_endpoint', code)

    def test_cloud_memory_gate_and_provider_parsing(self):
        code = (ROOT / 'src/gpt_bridge.cpp').read_text()
        self.assertIn('allow_cloud_memory_', code)
        self.assertIn('gpt_bridge::build_provider_request', code)
        self.assertIn('gpt_bridge::parse_provider_reply', code)
        self.assertIn('allow_cloud_api', code)

    def test_launch_exposes_provider_without_secret_parameters(self):
        code = (ROOT.parent / 'robot_bringup/launch/robot.launch.py').read_text()
        self.assertIn("DeclareLaunchArgument('provider', default_value='local'", code)
        self.assertIn("'allow_cloud_api': ParameterValue", code)
        self.assertIn("'allow_cloud_memory': ParameterValue", code)
        self.assertNotIn('ANTHROPIC_API_KEY', code)
        self.assertNotIn('OPENAI_API_KEY', code)
        self.assertIn("'max_tokens': ParameterValue(max_tokens, value_type=int)", code)
        self.assertIn("'model': ParameterValue(model, value_type=str)", code)
        self.assertIn("'endpoint': ParameterValue(endpoint, value_type=str)", code)

    def test_cloud_is_off_by_default_and_has_no_silent_fallback(self):
        code = (ROOT / 'src/gpt_bridge.cpp').read_text()
        self.assertIn('declare_parameter<std::string>("provider", "local")', code)
        self.assertIn('declare_parameter<bool>("allow_cloud_api", false)', code)
        self.assertIn('declare_parameter<bool>("allow_cloud_memory", false)', code)
        self.assertIn('(provider_ == "local" || allow_cloud_memory_)', code)
        launch = (ROOT.parent / 'robot_bringup/launch/robot.launch.py').read_text()
        self.assertIn("DeclareLaunchArgument('model', default_value=''", launch)
        self.assertIn("DeclareLaunchArgument('endpoint', default_value=''", launch)
        self.assertIn("DeclareLaunchArgument('allow_cloud_api', default_value='False'", launch)
        self.assertIn("DeclareLaunchArgument('allow_cloud_memory', default_value='False'", launch)

    def test_every_curl_option_is_checked(self):
        code = (ROOT / 'src/gpt_bridge.cpp').read_text()
        self.assertEqual(code.count('curl_easy_setopt('), 1)
        self.assertIn('curl_easy_setopt(handle, option, value) != CURLE_OK', code)
        self.assertIn('curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK', code)


if __name__ == '__main__':
    unittest.main()
