#!/usr/bin/env python

from glob import glob

import struct
import os
import fcntl
import sys
import errno
import time

# ################################# PARSING #################################

# event ID for schedcat job completions
ST_JOB_COMPLETION = 506

# How much data in a binary completion record?
ST_JOB_COMPLETION_LEN = 24 # bytes

class JobCompletionRecord(object):
    def __init__(self, raw_data):
        # unpack from raw data
        (self.event_id,
         self.cpu,
         self.task_id,
         self.job_id,
         self.when,
         self.exec_time
         ) = struct.unpack("BBHIQQ", raw_data[:ST_JOB_COMPLETION_LEN])

        # The exec_time field LSB is used as a flag.
        # The bit is set if the job completion was forced, i.e.,
        # if a budget overrun occurred.
        self.was_forced = self.exec_time & 0x1
        self.exec_time  = self.exec_time >> 1

    def __str__(self):
        return "[task %d/%d: completed at time %d on CPU %d, acet=%d, forced=%d]" \
            % (self.task_id, self.job_id, self.when, self.cpu, self.exec_time,
               self.was_forced)

def completion_records(data):
    return [JobCompletionRecord(data[(i * ST_JOB_COMPLETION_LEN):])
            for i in xrange(len(data) / ST_JOB_COMPLETION_LEN)]

def parse_st_job_completion(data):
    header, when, exec_time = struct.unpack("QQQ", data[:ST_JOB_COMPLETION_LEN])
    return (header, when, exec_time)


# ############################ DEVICE FILE HANDLING #########################

# ioctl() interface
ENABLE_CMD  = 0x0
DISABLE_CMD	= 0x1

def turn_event_on(file, event_id):
    return fcntl.ioctl(file, ENABLE_CMD, event_id)

def turn_event_off(file, event_id):
    return fcntl.ioctl(file, DISABLE_CMD, event_id)

def open_trace_file(fname):
    print 'Tracing %s...' % fname

    # open, without buffering
    file = open(fname, 'rb', 0)

    # turn on job completion records
    turn_event_on(file, ST_JOB_COMPLETION)

    # enable non-blocking I/O
    flags = fcntl.fcntl(file, fcntl.F_GETFL)
    fcntl.fcntl(file, fcntl.F_SETFL, flags | os.O_NONBLOCK)

    return file

def read_trace_records(file):
    try:
        data = file.read(8 * ST_JOB_COMPLETION_LEN) # read up to 16 trace records
    except IOError, err:
        if err.errno == errno.EWOULDBLOCK:
            # ok, non-blocking read didn't get any data
            data = []
        else:
            # something else went wrong, rethrow
            raise err

    # We are not expecting partial records.
    assert len(data) % ST_JOB_COMPLETION_LEN == 0

    return completion_records(data)

# ############################ MAIN SCRIPT #########################

def on_job_completion(record):
    print record

def monitor(trace_file_paths=sys.argv[1:], delay=0.5):
    if not trace_file_paths:
        trace_file_paths = glob('/dev/litmus/sched_trace*')

    files = [open_trace_file(fname) for fname in trace_file_paths]

    while True:
        for f in files:
            for rec in read_trace_records(f):
                on_job_completion(rec)
        time.sleep(delay)


if __name__ == '__main__':
    monitor()
