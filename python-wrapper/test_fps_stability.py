#!/usr/bin/env python3
"""Test FPS stability and subpage alternation over 10 seconds"""

import mlx90640
import numpy as np
import time

camera = mlx90640.MLX90640Camera()
camera.init()

print('Monitoring for 10 seconds to detect ghosting/FPS drops...')
print('Expected: consistent ~15.7 FPS, perfect subpage alternation')
print('-' * 70)

frame_times = []
subpages = []
fps_samples = []

start_time = time.time()
frame_count = 0

while time.time() - start_time < 10:
    frame_start = time.time()
    frame = camera.get_frame()
    frame_time = time.time() - frame_start
    frame_times.append(frame_time)

    subpage = camera.get_subpage_number()
    subpages.append(subpage)

    frame_count += 1
    elapsed = time.time() - start_time

    # Calculate rolling FPS
    if len(frame_times) >= 10:
        recent_fps = 1.0 / np.mean(frame_times[-10:])
        fps_samples.append(recent_fps)
    else:
        recent_fps = frame_count / elapsed if elapsed > 0 else 0

    # Print every 20 frames
    if frame_count % 20 == 0:
        recent_subpages = ''.join(str(s) for s in subpages[-20:])
        print(f'Frame {frame_count:3d}: FPS {recent_fps:5.2f} | Last 20 subpages: {recent_subpages}')

print()
print('=' * 70)
print(f'Total frames captured: {frame_count}')
print(f'Average FPS: {frame_count / 10:.2f}')
if fps_samples:
    print(f'FPS std dev: {np.std(fps_samples):.2f}')
print()

# Check for subpage alternation
alternating = all(subpages[i] != subpages[i-1] for i in range(1, len(subpages)))
if alternating:
    print('✓ PERFECT: Subpages alternated correctly for all frames!')
    print('  No ghosting detected!')
else:
    # Find where alternation broke
    for i in range(1, len(subpages)):
        if subpages[i] == subpages[i-1]:
            print(f'✗ FAILED: Subpage {subpages[i]} repeated at frame {i}')
            print(f'  Sequence: ...{"".join(str(s) for s in subpages[max(0,i-5):i+5])}...')
            break

camera.cleanup()
