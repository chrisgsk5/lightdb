import subprocess
from scannerpy import Database, DeviceType, Job, BulkJob
from scannerpy.stdlib import NetDescriptor
from scannerpy.stdlib.video import write_video
import numpy as np
import cv2
import struct
import sys
import os
sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/..')
sys.path.append('/opt/scanner/examples')
sys.path.append('/opt/scanner')
import util

video_path = '/app/foo.mp4' if len(sys.argv) <= 1 else sys.argv[1]
video_name = os.path.splitext(os.path.basename(video_path))[0]

print video_path


with Database() as db:
    [input_table], _ = db.ingest_videos(
        [('video1', video_path)], force=True)

    frame = db.ops.FrameInput()
    id = db.ops.identity(frame=frame, device=DeviceType.GPU)
    output = db.ops.Output(columns=[id])

    job = Job(op_args={
        frame: input_table.column('frame'),
        output: input_table.name() + '_output'
    })
    bulk_job = BulkJob(output=output, jobs=[job])

    [output] = db.run(bulk_job, pipeline_instances_per_node=-1, tasks_in_queue_per_pu=10, force=True)
    frames = [f[1][0] for f in output.load(['frame_output'])]
    i, delta = 0, 45
    while frames:
        current = frames[:delta]
        write_video('out{}.mp4'.format(i), current, 30)
        i += 1
        frames = frames[delta:]
