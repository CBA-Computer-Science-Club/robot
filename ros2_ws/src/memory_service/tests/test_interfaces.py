from pathlib import Path
import unittest

PACKAGE = Path(__file__).resolve().parents[1]


class ServiceContractTest(unittest.TestCase):
    def test_service_schemas(self):
        expected = {
            'AddMemory.srv': 'string person_id\nstring key\nstring value\nbool consent\n---\nbool success\nstring message\n',
            'GetMemory.srv': 'string person_id\nstring key\nbool consent\n---\nbool found\nstring value\nstring message\n',
            'ListMemories.srv': 'string person_id\nuint32 limit\nbool consent\n---\nbool success\nstring[] keys\nstring[] values\nstring message\n',
            'ForgetPerson.srv': 'string person_id\n---\nbool success\nstring message\n',
        }
        for filename, schema in expected.items():
            with self.subTest(filename=filename):
                self.assertEqual((PACKAGE / 'srv' / filename).read_text(), schema)

    def test_generated_and_wired(self):
        cmake = (PACKAGE / 'CMakeLists.txt').read_text()
        node = (PACKAGE / 'src' / 'memory_service.cpp').read_text()
        for filename in ('AddMemory', 'GetMemory', 'ListMemories', 'ForgetPerson'):
            with self.subTest(filename=filename):
                self.assertIn(f'"srv/{filename}.srv"', cmake)
        for service in ('memory/add', 'memory/get', 'memory/list', 'memory/forget'):
            with self.subTest(service=service):
                self.assertIn(f'"{service}"', node)


if __name__ == '__main__':
    unittest.main()
