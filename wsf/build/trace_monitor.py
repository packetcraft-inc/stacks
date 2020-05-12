###################################################################################################
#
# Trace monitor
#
# Copyright (c) 2016-2019 Arm Ltd. All Rights Reserved.
#
# Copyright (c) 2019 Packetcraft, Inc.
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
###################################################################################################

import os, sys
import platform
import serial
import csv
import datetime
import struct
import argparse

# -------------------------------------------------------------------------------------------------
#     Constants
# -------------------------------------------------------------------------------------------------

DEF_BAUD = 115200

class Color:
    OFF = '\033[0;m'
    RED = '\033[0;31m'
    YELLOW = '\033[0;33m'
    GREEN = '\033[0;32m'
    GREY = '\033[0;37m'

ENABLE_COLOR_CODING = True

# -------------------------------------------------------------------------------------------------
#     Functions
# -------------------------------------------------------------------------------------------------

def ParseArgs():
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='Monitor serial port and output detokenized messages')
    parser.add_argument('-b', '--baud', metavar='RATE', help='baud rate (e.g. "115200")', default=DEF_BAUD, type=int)
    parser.add_argument('-d', '--device', metavar='DEV', help='serial device name (e.g. "COM1" or "/dev/ttyUSB0")', required=True)
    parser.add_argument('-p', '--pass_filter', metavar='STR', help='pass filter strings, comma delimited, case sensitive', default='')
    parser.add_argument('token_files', metavar='FILE', help='filename and path of token files', nargs='+')

    args = parser.parse_args()
    args = vars(args)

    if not args['baud']:
        args['baud'] = DEF_BAUD

    if args['pass_filter']:
        args['pass_filter'] = args['pass_filter'].split(',')
    else:
        args['pass_filter'] = []

    return args

def GetTokens(token_files):
    tokens = {}
    sources = {}

    for token_file in token_files:
        try:
            f = open(token_file)
            text = f.read()
            f.close()
        except:
            print >> sys.stderr, 'error: could not open token definition file "%s"' % token_file
            sys.exit(-1)

        line_no = 0
        for f in csv.reader(text.split('\n'), skipinitialspace=True):
            line_no += 1

            if not f:
                continue

            [MODULE_ID, LINE, FILE, SUBSYS, STATUS, MSG] = range(6)

            try:
                line = int(f[LINE])
                module_id = int(f[MODULE_ID], 16)
                line = int(f[LINE])
                filename = os.path.basename(f[FILE])

                if line > 0:
                    token = (line << 16) | module_id
                    source = '%s:%u' % (filename, line)
                    subsys = f[SUBSYS]
                    status = f[STATUS]
                    msg = f[MSG]

                    tokens[token] = {
                        'source': source,
                        'status': status,
                        'subsys': subsys,
                        'msg': msg}
                else:
                    sources[module_id] = filename
            except:
                print >> sys.stderr, 'error: invalid token definition at line %u' % line_no

    return tokens, sources

def ColorPrint(color, msg):
    if ENABLE_COLOR_CODING and sys.stdout.isatty():
        print color + msg + Color.OFF
    else:
        print msg

    sys.stdout.flush()

# -------------------------------------------------------------------------------------------------
#     Objects
# -------------------------------------------------------------------------------------------------

class TraceDevice:
    def __init__(self, token_files, dev_name, baud, pass_filter=[]):
        self.token_files = token_files
        self.dev_name = dev_name
        self.pass_filter = pass_filter

        if platform.system() == 'Linux':
            # Workaround to ensure Linux configure serial device correctly
            os.system('stty -F %s speed %d > /dev/null' % (dev_name, baud))

        self.tokens, self.str_lookup = GetTokens(token_files)
        self.serial = serial.Serial(dev_name, baudrate=baud, rtscts=False, timeout=0.1)

        self.stats = { 'INFO': 0, 'WARN': 0, 'ERR': 0 }
        self.msg_cnt = 1
        self.num_filtered = 0

        self.serial.flushInput()

    def Close(self):
        self.serial.close()

    def WaitForSync(self):
        msg = ''

        sys.stdout.write('Waiting for device initialization...')
        sys.stdout.flush()

        try:
            while True:
                msg += self.serial.read(8 - len(msg))

                if len(msg) == 8:
                    sync_word, = struct.unpack('<Q', msg)
                    if sync_word == 0xFFFFFFFFFFFFFFFF:
                        break
                    else:
                        # Remove first character
                        msg = msg[1:]

        except KeyboardInterrupt:
            sys.stdout.write('cancelled.\n')
            sys.stdout.flush()
            sys.exit(0)

        sys.stdout.write('done.\n')
        sys.stdout.flush()

    def GetParamStr(self, module_id):
        return self.str_lookup[module_id] if module_id in self.str_lookup else '<0x%04x>' % module_id

    def FormatTraceLog(self, token, param=0, flags=0):
        if type(token) == str:
            msg = token
            token = {
                'source': '<internal>',
                'status': 'ERR',
                'subsys': '---',
                'msg': msg}

        trace_log  = '%7d%s| ' % (self.msg_cnt, '*' if flags else ' ')
        trace_log += '%-30s | ' % token['source']
        trace_log += '%-3s | ' % token['subsys']
        trace_log += '%-4s | ' % token['status']

        num_param = token['msg'].count('%')
        num_str = token['msg'].count('%s')

        if num_str:
            if num_param == 1:
                trace_log += token['msg'] % (self.GetParamStr(param & 0xFFFF))
            elif num_param == 2 and num_str == 1:
                if token['msg'].index('%s') == token['msg'].index('%'):
                    trace_log += token['msg'] % (self.GetParamStr(param & 0xFFFF), (param >> 16))
                else:
                    trace_log += token['msg'] % ((param & 0xFFFF), self.GetParamStr(param >> 16))
            elif num_param == 2 and num_str == 2:
                trace_log += token['msg'] % (self.GetParamStr(param & 0xFFFF), self.GetParamStr(param >> 16))
            else:
                trace_log += token['msg']
        else:
            if num_param == 1:
                trace_log += token['msg'] % (param & 0xFFFFFFFF)
            elif num_param == 2:
                trace_log += token['msg'] % ((param & 0xFFFF), (param >> 16))
            elif num_param == 3:
                trace_log += token['msg'] % ((param & 0xFF), ((param >> 8) & 0xFF), ((param >> 16) & 0xFFFF))
            elif num_param == 4:
                trace_log += token['msg'] % ((param & 0xFF), ((param >> 8) & 0xFF), ((param >> 16) & 0xFF), ((param >> 24) & 0xFF))
            else:
                trace_log += token['msg']

        return trace_log

    def GetTraceMsg(self):
        msg = ''

        while (len(msg) < 8):
            msg += self.serial.read(8 - len(msg))

        return msg

    def ParseTraceMsg(self, msg, tstamp):
        (token, param) = struct.unpack('<II', msg)

        flags = token >> 28
        token = token & 0x0FFFFFFF
        if token in self.tokens:
            status = self.tokens[token]['status']
            trace_log = self.FormatTraceLog(self.tokens[token], param, flags)
        elif flags == 0xF and token == 0x0FFFFFFF and param == 0xFFFFFFFF:
            self.tokens, self.str_lookup = GetTokens(self.token_files)      # reload tokens
            self.start_time = datetime.datetime.now()
            self.last_tstamp = None
            dev.ResetBanner()
            return                                  # exit; no further processing
        else:
            status = 'ERR'
            err_msg = 'undefined message token=0x%08x, param=0x%08x' % (token, param)
            trace_log = self.FormatTraceLog(err_msg)

        if not self.last_tstamp:
            trace_log = '+00:00:00 | ' + trace_log
            self.last_tstamp = tstamp
        elif (tstamp - self.last_tstamp).seconds > 0:
            h, rem = divmod((tstamp - self.start_time).seconds, 3600)
            m, s = divmod(rem, 60)
            fdtime = '+%02u:%02u:%02u' % (h, m, s)
            #ftstamp = tstamp.strftime('%I:%M:%S %p')
            trace_log = fdtime + ' | ' + trace_log
            self.last_tstamp = tstamp
        else:
            trace_log = (' ' * len('+12:34:56')) + ' | ' + trace_log

        if self.pass_filter and not any(word in trace_log for word in self.pass_filter):
            self.num_filtered += 1
            return                                  # exit; no further processing

        if status == 'INFO':
            ColorPrint(Color.OFF, trace_log)
        elif status == 'WARN':
            ColorPrint(Color.YELLOW, trace_log)
        elif status == 'ERR':
            ColorPrint(Color.RED, trace_log)
        else:
            ColorPrint(Color.GREY, trace_log)

        self.msg_cnt += 1

        if status in self.stats:
            self.stats[status] += 1
        else:
            self.stats[status] = 0

    def Monitor(self):
        self.start_time = datetime.datetime.now()
        self.last_tstamp = None

        self.StartBanner()

        try:
            while True:
                raw_msg = self.GetTraceMsg()
                tstamp = datetime.datetime.now()
                self.ParseTraceMsg(raw_msg, tstamp)
        except KeyboardInterrupt:
            pass

        self.FinishBanner()

    def StartBanner(self):
        print 140 * '='
        print 'token_file  = %s (%u tokens)' % (self.token_files, len(self.tokens))
        print 'serial_dev  = %s' % self.dev_name
        print 'pass_filter = %s' % (self.pass_filter if self.pass_filter else '<all>')
        print 'start_time  = %s' % self.start_time.strftime('%I:%M%p %Z on %b %d, %Y')
        print 140 * '='

    def ResetBanner(self):
        print 140 * '='
        print 'token_file  = %s (%u tokens)' % (self.token_files, len(self.tokens))
        print 'reset_time  = %s' % datetime.datetime.now().strftime('%I:%M%p %Z on %b %d, %Y')
        print 140 * '='

    def FinishBanner(self):
        print 140 * '='
        print 'trace_stats  = %s' % str(self.stats)
        print 'num_filtered = %s' % self.num_filtered
        print 'start_time  = %s' % self.start_time.strftime('%I:%M%p %Z on %b %d, %Y')
        print 'finish_time  = %s' % datetime.datetime.now().strftime('%I:%M%p %Z on %b %d, %Y')
        print 140 * '='

# -------------------------------------------------------------------------------------------------
#     Main
# -------------------------------------------------------------------------------------------------

if (platform.system() == 'Windows') or (not sys.stdout.isatty()):
    # Force disable if not supported
    ENABLE_COLOR_CODING = False

if __name__ == '__main__':
    args = ParseArgs()

    dev = TraceDevice(args['token_files'], args['device'], args['baud'], args['pass_filter'])
    dev.WaitForSync()

    dev.Monitor()

    dev.Close()
