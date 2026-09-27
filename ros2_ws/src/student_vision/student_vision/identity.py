"""Consent-gated local template store. No network access or identity-topic ingestion."""
import json
import os
import re
import tempfile
from pathlib import Path

_ID = re.compile(r"[A-Za-z0-9][A-Za-z0-9_-]{0,63}\Z")
_PNG = b"\x89PNG\r\n\x1a\n"


def validate_id(person_id):
    if not isinstance(person_id, str) or not _ID.fullmatch(person_id):
        raise ValueError("person_id must be 1-64 ASCII letters/digits/_/- and start alphanumeric")
    return person_id


def _atomic_write(path, data):
    fd, temp = tempfile.mkstemp(prefix=".template-", dir=path.parent)
    try:
        if os.name == "posix":
            os.fchmod(fd, 0o600)
        with os.fdopen(fd, "wb") as output:
            output.write(data)
        os.replace(temp, path)
    finally:
        if os.path.exists(temp):
            os.unlink(temp)


class TemplateStore:
    def __init__(self, root: Path):
        self.root = Path(root).expanduser()

    def _paths(self, person_id):
        person_id = validate_id(person_id)
        return self.root / (person_id + ".png"), self.root / (person_id + ".json")

    def list_people(self):
        if not self.root.exists():
            return {}
        people = {}
        for metadata in self.root.glob("*.json"):
            try:
                person_id = validate_id(metadata.stem)
                image, _ = self._paths(person_id)
                if metadata.is_symlink() or image.is_symlink():
                    continue
                record = json.loads(metadata.read_text(encoding="utf-8"))
                if record == {"person_id": person_id, "consent": True}:
                    template = image.read_bytes()
                    if template.startswith(_PNG):
                        people[person_id] = template
            except (OSError, ValueError, UnicodeError):
                continue
        return people

    def enroll(self, person_id, image, *, consent):
        path, metadata = self._paths(person_id)
        if consent is not True:
            raise ValueError("Explicit consent required")
        if not isinstance(image, bytes) or not image.startswith(_PNG):
            raise ValueError("Expected validated, cropped PNG face template")
        self.root.mkdir(mode=0o700, parents=True, exist_ok=True)
        if path.is_symlink() or metadata.is_symlink():
            raise ValueError("Refusing symlink target")
        _atomic_write(path, image)
        _atomic_write(metadata, json.dumps({"person_id": person_id, "consent": True}).encode())

    def delete(self, person_id):
        path, metadata = self._paths(person_id)
        existed = path.exists() or metadata.exists()
        # Remove authorization before image, so concurrent scans fail closed.
        metadata.unlink(missing_ok=True)
        path.unlink(missing_ok=True)
        return existed
