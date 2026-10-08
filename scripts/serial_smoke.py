"""Optional board bench test. Starts/stops mock playback; do not touch UI meanwhile."""
import argparse
import re
import statistics
import time
from pathlib import Path

import serial

parser = argparse.ArgumentParser()
parser.add_argument('--port', default='COM3')
parser.add_argument('--seconds', type=float, default=8)
args = parser.parse_args()
lines = []


def collect(connection, duration):
    end = time.monotonic() + duration
    while time.monotonic() < end:
        line = connection.readline().decode(errors='replace').strip()
        if line:
            lines.append(line)


with serial.Serial(args.port, 115200, timeout=0.15) as connection:
    connection.rts = False
    connection.dtr = True
    connection.write(b's')
    collect(connection, 1)
    lines.clear()
    connection.write(b'p')
    try:
        collect(connection, args.seconds)
    finally:
        connection.write(b's')
        collect(connection, 1.5)

Path('build').mkdir(exist_ok=True)
Path('build/serial-smoke.log').write_text('\n'.join(lines) + '\n', encoding='utf-8')
events = []
for line in lines:
    match = re.fullmatch(r'MOCK t=(\d+) (ON|OFF) ch=1 note=(\d+) vel=(\d+)', line)
    if match:
        events.append((int(match[1]), match[2], int(match[3]), int(match[4])))
    assert 'INIT ERROR' not in line and 'Guru Meditation' not in line, line

assert len(events) >= 32, f'Too few events: {len(events)}; see build/serial-smoke.log'
active = None
onsets, gates = [], []
for at, kind, note, velocity in events:
    if kind == 'ON':
        assert active is None, 'Overlapping notes or missing OFF'
        assert 0 <= note <= 127 and 1 <= velocity <= 127
        active = (note, at)
        onsets.append(at)
    else:
        assert active is not None and active[0] == note, 'Wrong or unmatched OFF'
        gates.append(at - active[1])
        active = None
assert active is None, 'Stop did not release final note'
intervals = [b - a for a, b in zip(onsets, onsets[1:])]
assert all(abs(interval - 125000) < 10000 for interval in intervals), intervals
# Final gate may be shortened by Stop; all other gates target 75% at 120 BPM.
assert all(abs(gate - 93750) < 10000 for gate in gates[:-1]), gates
print(f'PASS: {len(onsets)} ON/OFF pairs, stop released final note')
print(f'Step us: min={min(intervals)}, median={statistics.median(intervals)}, max={max(intervals)}')
print(f'Gate us (excluding stop): min={min(gates[:-1])}, max={max(gates[:-1])}')
for line in lines:
    if line.startswith('UI alive'):
        print(line)
