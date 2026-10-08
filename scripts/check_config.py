"""Offline sanity checks; does not replace PlatformIO's firmware compiler."""
import configparser
import csv
from pathlib import Path

root = Path(__file__).resolve().parents[1]
config = configparser.ConfigParser()
config.read(root / "platformio.ini")
assert config["env:waveshare_s3"]["board_build.arduino.memory_type"] == "qio_opi"
end = 0x9000
for row in csv.reader((root / "partitions.csv").read_text().splitlines()):
    if not row or row[0].startswith("#"):
        continue
    offset, size = int(row[3], 0), int(row[4], 0)
    assert offset >= end and size > 0
    assert offset % 0x1000 == 0
    if row[1].strip() == "app":
        assert offset % 0x10000 == 0
    end = offset + size
assert end == 16 * 1024 * 1024
print("PASS: configuration parsed; partitions aligned, non-overlapping, within 16 MB")
