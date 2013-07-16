#!/usr/bin/python
#
# Copyright (c) 2012, Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# - Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# - Neither the name of the University of California, Berkeley nor the names
#   of its contributors may be used to endorse or promote products derived
#   from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
#
# Run test suite
#
# by Fernando L. Garcia Bermudez
#
# v.0.1
#
# Revisions:
#  Fernando L. Garcia Bermudez      2012-8-20    Initial release
#

import msvcrt, sys, traceback
import test_suite
import time


#RADIO_DEV_NAME  = '/dev/tty.usbserial-*' or 'COMx'
#RADIO_DEV_NAME = 'COM1'
RADIO_DEV_NAME = 'COM4'
BS_BAUDRATE = 230400

#DEST_ADDR = '\x21\x02'
DEST_ADDR = '\x20\x52'

motorgains = [1800,20,100,0,0, 1800,20,100,0,0]
duration = 2000


if __name__ == '__main__':
    try:
        ts = test_suite.TestSuite(RADIO_DEV_NAME,            \
                                  baud_rate=BS_BAUDRATE, \
                                  dest_addr=DEST_ADDR  )

        #Initialization
        ts.SetGains(motorgains)


        while msvcrt.kbhit():
            ch = msvcrt.getch()

        while True:
            keypress = msvcrt.getch()
            print '>',

            if keypress == 'p':
                ts.PIDStart(duration)

            elif keypress == ' ':
                ts.PIDSTAHP()

            elif keypress == 'm':
                ts.test_motorop()

            elif keypress == 'z':
                ts.zeroPos()

            elif keypress == 'w':
                ts.test_mpu()
            elif keypress == 'd':
                ts.test_dflash()
            elif keypress == 't':
                print 'Current duration '+str(duration)+', New duration in ms:',
                duration = int(raw_input())    
                print 'Current duration '+str(duration)
            elif keypress == 'r':
                print "Turn test. Enter leg frequency:",
                p = 1000.0/int(raw_input())
                ts.defProfile([100,0,0,0,0,100,0,0,0,0])
                ts.PIDStart(duration)
                vel = [int(p), 0x4000>>2, 0x4000>>2, 0x4000>>2, 0x4000>>2, int(p), 0x4000>>2, 0x4000>>2, 0x4000>>2, 0x4000>>2]
                ts.defProfile(vel)
                time.sleep(4.9*p/1000)
                vel = [int(p), 0x4000>>2, 0x4000>>2, 0x4000>>2, 0x4000>>2, int(p), 0x2000>>2, 0x2000>>2, 0x8000>>2, 0x4000>>2]
                ts.defProfile(vel)
                time.sleep(0.9*p/1000)
                vel = [int(p), 0x4000>>2, 0x4000>>2, 0x4000>>2, 0x4000>>2, int(p), 0x4000>>2, 0x4000>>2, 0x4000>>2, 0x4000>>2]
                ts.defProfile(vel)
                time.sleep(5*p/1000)
                ts.PIDSTAHP()
                print "Done"
            elif keypress == 'g':
                ts.SetGains()

            elif keypress == 'i':
                ts.SetProfile()
            elif keypress == 'a':
                ts.test_amspos()
            elif keypress == 'q':
                ts.quit()
            else:
                print "** unknown keyboard command** \n"
                

    


        ts.__del__()
        sys.exit(0)
    
    ### Exception handling
    except SystemExit as e:
        print('\nI: SystemExit: ' + str(e))
        sys.exit(1)
    except KeyboardInterrupt:
        print('\nI: KeyboardInterrupt')
        ts.__del__()
        sys.exit(1)
    except Exception as e:
        print('\nE: Unexpected exception!\n' + str(e))
        traceback.print_exc()
        sys.exit(1)
