#include "cmd.h"
#include "cmd_const.h"
#include "dfmem.h"
#include "utils.h"
#include "ports.h"
#include "sclock.h"
#include "led.h"
#include "blink.h"
#include "payload.h"
#include "mac_packet.h"
#include "dfmem.h"
#include "radio.h"
#include "dfmem.h"
#include "tests.h"
#include "version.h"
#include "settings.h"
#include "timer.h"
#include "tih.h"
#include "pid-ip2.5.h"
#include "ams-enc.h"
#include "carray.h"
#include "telem.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

unsigned char (*cmd_func[MAX_CMD_FUNC])(unsigned char, unsigned char, unsigned char, unsigned char*);
void cmdError(void);

extern pidPos pidObjs[NUM_PIDS];
extern EncObj encPos[NUM_ENC];
extern volatile CircArray fun_queue;
unsigned long packetNum = 0; // sequential packet number
extern telemStruct_t telemPIDdata; 

/*-----------------------------------------------------------------------------
 *          Declaration of static functions
-----------------------------------------------------------------------------*/
static unsigned char cmdNop(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame);
static unsigned char cmdEcho(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame);
static unsigned char cmdSoftwareReset(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame);
static unsigned char cmdWhoAmI(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame);
static unsigned char cmdGetAMSPos(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame);

//Motor and PID functions
static unsigned char cmdSetThrustOpenLoop(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame);
static unsigned char cmdSetThrustClosedLoop(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame);
static unsigned char cmdSetPIDGains(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame);
static unsigned char cmdPIDStartMotors(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame);
static unsigned char cmdPIDStopMotors(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame);
static unsigned char cmdSetVelProfile(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame);
static unsigned char cmdZeroPos(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame);

//Experiment/Flash Commands
static unsigned char cmdGetPIDTelemetry(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame);
static unsigned char cmdStartTimedRun(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame);
static unsigned char cmdStartTelemetry(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame);
static unsigned char cmdEraseSectors(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame);
static unsigned char cmdFlashReadback(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame);
/*-----------------------------------------------------------------------------
 *          Public functions
-----------------------------------------------------------------------------*/
void cmdSetup(void) {

    unsigned int i;

    // initialize the array of func pointers with Nop()
    for(i = 0; i < MAX_CMD_FUNC; ++i) {
        cmd_func[i] = &cmdNop;
    }
    cmd_func[CMD_TEST_RADIO] = &test_radio;
    cmd_func[CMD_TEST_MPU] = &test_mpu;
    cmd_func[CMD_ECHO] = &cmdEcho;
    cmd_func[CMD_SOFTWARE_RESET] = &cmdSoftwareReset;
    cmd_func[CMD_SET_THRUST_OPEN_LOOP] = &cmdSetThrustOpenLoop;
    cmd_func[CMD_SET_THRUST_CLOSED_LOOP] = &cmdSetThrustClosedLoop;
    cmd_func[CMD_PID_START_MOTORS] = &cmdPIDStartMotors;
    cmd_func[CMD_SET_PID_GAINS] = &cmdSetPIDGains;
    cmd_func[CMD_GET_AMS_POS] = &cmdGetAMSPos;
    cmd_func[CMD_START_TELEMETRY] = &cmdStartTelemetry;
    cmd_func[CMD_ERASE_SECTORS] = &cmdEraseSectors;
    cmd_func[CMD_FLASH_READBACK] = &cmdFlashReadback;
    cmd_func[CMD_SET_VEL_PROFILE] = &cmdSetVelProfile;
    cmd_func[CMD_WHO_AM_I] = &cmdWhoAmI;
    cmd_func[CMD_ZERO_POS] = &cmdZeroPos;   
    cmd_func[CMD_START_TIMED_RUN] = &cmdStartTimedRun;
    cmd_func[CMD_PID_STOP_MOTORS] = &cmdPIDStopMotors;
// *ADDED*
    cmd_func[CMD_GET_PID_TELEMETRY] = cmdGetPIDTelemetry;

}

void cmdPushFunc(MacPacket rx_packet) {
    Payload rx_payload;
    unsigned char command;

    rx_payload = macGetPayload(rx_packet);
    if(rx_payload != NULL) {
        command = payGetType(rx_payload);

        if(command < MAX_CMD_FUNC && cmd_func[command] != NULL) {
            rx_payload->test = cmd_func[command];
            carrayAddTail(fun_queue, rx_packet);
        } else {
            cmdError();   // halt on error - could also just ignore....
        }
    }
}

// simple echo response
unsigned char cmdEcho(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame) 
{       radioSendData(RADIO_DEST_ADDR, status, CMD_ECHO, length, frame,0);
    return 1; //success
}

unsigned char cmdSoftwareReset(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame) 
{     	asm volatile("reset");
	return 1;
}

// send robot info when queried
unsigned char cmdWhoAmI(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame) {
    unsigned char i, string_length; unsigned char *version_string;
    // maximum string length to avoid packet size limit
    version_string = versionGetString();
    i = 0;
    while((i < 127) && version_string[i] != '\0') {
        i++;
    }
    string_length=i;
    radioSendData(RADIO_DEST_ADDR, status, CMD_WHO_AM_I,
            string_length, version_string, 0);
    return 1; //success
}

unsigned char cmdGetAMSPos(unsigned char type, unsigned char status,
        unsigned char length, unsigned char *frame) {
    long motor_count[2];
    motor_count[0] = pidObjs[0].p_state;
    motor_count[1] = pidObjs[1].p_state;

    // motor_count[0] = encPos[0].pos;
    // motor_count[1] = encPos[1].pos;

    radioSendData(RADIO_DEST_ADDR, status, CMD_GET_AMS_POS,
            sizeof(motor_count), (unsigned char *)motor_count, 0);
    return 1;
}
// ==== Flash/Experiment Commands ==============================================================================
// =============================================================================================================

unsigned char cmdStartTimedRun(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame){
    unsigned int run_time = frame[0] + (frame[1] << 8);
    int i;
    for (i = 0; i < NUM_PIDS; i++){
        pidObjs[i].timeFlag = 1;
        pidSetInput(i, 0,run_time);
        pidObjs[i].p_input = pidObjs[i].p_state;
        pidOn(i);
    }

    pidStartTimedTrial(run_time,run_time);

    return 1;
}
unsigned char cmdStartTelemetry(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame){
    unsigned int numSamples = frame[0] + (frame[1] << 8);
    if (numSamples != 0) {
        telemSetStartTime(); // Start telemetry samples from approx 0 time
        telemSetSamplesToSave(numSamples);
    }
    return 1;
}
unsigned char cmdEraseSectors(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame){
    unsigned int numSamples = frame[0] + (frame[1] << 8);
    telemErase(numSamples);
    LED_RED = ~LED_RED;
    return 1;
}
unsigned char cmdFlashReadback(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame){
    unsigned int numSamples = frame[0] + (frame[1] << 8);
    telemReadbackSamples(numSamples);
    return 1;
}

unsigned char cmdGetPIDTelemetry(unsigned char type, unsigned char status, unsigned char length,unsigned char *frame)
{ unsigned int sampLen = sizeof(telemStruct_t);
	telemGetPID(); // get current state
	telemPIDdata.sampleIndex = packetNum;
	radioSendData(RADIO_DEST_ADDR, status, CMD_SPECIAL_TELEMETRY, 
					sampLen, (unsigned char *) &telemPIDdata,0);

	packetNum++;
	LED_GREEN = ~ LED_GREEN;
	LED_RED = ~ LED_RED;
	LED_BLUE = ~LED_BLUE;
	return 1;
}

// ==== Motor PID Commands =====================================================================================
// =============================================================================================================

unsigned char cmdSetThrustOpenLoop(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame) {
    int thrust1 = frame[0] + (frame[1] << 8);
    int thrust2 = frame[2] + (frame[3] << 8);
    unsigned int run_time_ms = frame[4] + (frame[5] << 8);

    DisableIntT1;   // since PID interrupt overwrites PWM values

    tiHSetDC(1, thrust1);
    tiHSetDC(2, thrust2);
    delay_ms(run_time_ms);
    tiHSetDC(1,0);
    tiHSetDC(2,0);

    EnableIntT1;
    return 1;
 } 


unsigned char cmdSetThrustClosedLoop(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame) {
    int thrust1 = frame[0] + (frame[1] << 8);
    unsigned int run_time_ms1 = frame[2] + (frame[3] << 8);
    int thrust2 = frame[4] + (frame[5] << 8);
    unsigned int run_time_ms2 = frame[6] + (frame[7] << 8);

	pidSetInput(0 ,thrust1, run_time_ms1);
	pidOn(0); 
	pidSetInput(1 ,thrust2, run_time_ms2);
	pidOn(1); 
	pidStartTimedTrial(run_time_ms1,run_time_ms2);
       return 1;
 } 

 unsigned char cmdSetPIDGains(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame) {
    int Kp, Ki, Kd, Kaw, ff;
    int idx = 0;

    Kp = frame[idx] + (frame[idx+1] << 8); idx+=2;
    Ki = frame[idx] + (frame[idx+1] << 8); idx+=2;
    Kd = frame[idx] + (frame[idx+1] << 8); idx+=2;
    Kaw = frame[idx] + (frame[idx+1] << 8); idx+=2;
    ff = frame[idx] + (frame[idx+1] << 8); idx+=2;
    pidSetGains(0,Kp,Ki,Kd,Kaw, ff);
    Kp = frame[idx] + (frame[idx+1] << 8); idx+=2;
    Ki = frame[idx] + (frame[idx+1] << 8); idx+=2;
    Kd = frame[idx] + (frame[idx+1] << 8); idx+=2;
    Kaw = frame[idx] + (frame[idx+1] << 8); idx+=2;
    ff = frame[idx] + (frame[idx+1] << 8); idx+=2;
    pidSetGains(1,Kp,Ki,Kd,Kaw, ff);

    radioSendData(RADIO_DEST_ADDR, status, CMD_SET_PID_GAINS, 20, frame, 0);
    //Send confirmation packet
    // WARNING: Will fail at high data throughput
    //radioConfirmationPacket(RADIO_DEST_ADDR, CMD_SET_PID_GAINS, status, 20, frame);
    return 1; //success
}

unsigned char cmdSetVelProfile(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame) {
    int interval[NUM_VELS], delta[NUM_VELS], vel[NUM_VELS], period;
    int idx = 0, i = 0;
    // Packet structure [Period, delta[NUM_VELS], Period, delta[NUM_VELS]]
    period = frame[idx] + (frame[idx + 1]<<8);
    idx+=2;
    for(i = 0; i < NUM_VELS; i ++) {
        interval[i] = period/NUM_VELS;
        delta[i] = (frame[idx]+ (frame[idx+1]<<8));
            if(delta[i]>=8192){
                delta[i] = 8191;
            } else if(delta[i] < -8192){
                delta[i] = -8192;
            }
        delta[i] = delta[i]<<2;
        vel[i] = delta[i]/interval[i];
        idx+=2;
    }
    setPIDVelProfile(0, interval, delta, vel);
    
    period = frame[idx] + (frame[idx + 1]<<8);
    idx+=2;
    for(i = 0; i < NUM_VELS; i ++) {
        interval[i] = period/NUM_VELS;
        delta[i] = (frame[idx]+ (frame[idx+1]<<8));
            if(delta[i]>=8192){
                delta[i] = 8191;
            } else if(delta[i] < -8192){
                delta[i] = -8192;
            }
        delta[i] = delta[i]<<2;
        vel[i] = delta[i]/interval[i];
        idx+=2;
        }

    setPIDVelProfile(1, interval, delta, vel);

    //Send confirmation packet
    // TODO : Send confirmation packet with packet index
    return 1; //success
}

unsigned char cmdPIDStartMotors(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame) {
    pidObjs[0].timeFlag = 0;
    pidObjs[1].timeFlag = 0;
    pidSetInput(0, 0,0);
    pidObjs[0].p_input = pidObjs[0].p_state;
    pidOn(0);
    pidSetInput(1, 0,0);
    pidObjs[1].p_input = pidObjs[1].p_state;
    pidOn(1);
    return 1;
}

unsigned char cmdPIDStopMotors(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame) {
    pidObjs[0].onoff = 0;
    pidObjs[1].onoff = 0;
    return 1;
}

unsigned char cmdZeroPos(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame) {
    long motor_count[2];
    motor_count[0] = pidObjs[0].p_state;
    motor_count[1] = pidObjs[1].p_state;

    radioSendData(RADIO_DEST_ADDR, status, CMD_ZERO_POS,
        sizeof(motor_count), (unsigned char *)motor_count, 0);
    pidZeroPos(0); pidZeroPos(1);
    return 1;
}

void cmdError() {
    int i;
    EmergencyStop();
    for(i= 0; i < 10; i++) {
        LED_1 ^= 1;
        delay_ms(200);
        LED_2 ^= 1;
        delay_ms(200);
        LED_3 ^= 1;
        delay_ms(200);
    }
}

static unsigned char cmdNop(unsigned char type, unsigned char status, unsigned char length, unsigned char *frame) {
    return 1;
}
