"""Explicit local enrollment and revocation; never enroll from ROS topics."""
import argparse
from pathlib import Path

from .identity import TemplateStore, validate_id
from .recognizer import FaceRecognizer


def build_parser():
    parser = argparse.ArgumentParser(description="Manage local consented face templates")
    parser.add_argument("--data-dir", type=Path, default=Path.home() / ".local/share/robot/student_vision")
    commands = parser.add_subparsers(dest="command", required=True)
    enroll = commands.add_parser("enroll")
    enroll.add_argument("--person-id", required=True)
    enroll.add_argument("--image", type=Path, required=True)
    enroll.add_argument("--consent", action="store_true", help="I confirm the person's explicit consent")
    delete = commands.add_parser("delete")
    delete.add_argument("--person-id", required=True)
    return parser


def main(argv=None):
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        person_id = validate_id(args.person_id)
        store = TemplateStore(args.data_dir)
        if args.command == "delete":
            store.delete(person_id)
            print("Local template deleted (if present); backups must be erased separately")
            return
        if not args.consent:
            parser.error("enrollment requires --consent from the person being enrolled")
        # Decode with imread (PNG/JPEG). Never copy the raw enrollment photograph to storage.
        backend = FaceRecognizer()
        image = backend.cv2.imread(str(args.image), backend.cv2.IMREAD_COLOR)
        if image is None:
            parser.error("Could not decode enrollment image")
        template = backend.enrollment_template(image)
        store.enroll(person_id, template, consent=True)
        print("Consented face template stored locally; source photograph remains caller-owned")
    except (ValueError, RuntimeError, OSError) as exc:
        parser.error(str(exc))
