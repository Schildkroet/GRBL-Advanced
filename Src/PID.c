#include "PID.h"
#include "System32.h"

#define TICK_SECOND     1000


extern uint32_t millis(void);


PID_t *PID_Create(PID_t *pid, float* in, float* out, float* set, float kp, float ki, float kd)
{
	pid->input = in;
	pid->output = out;
	pid->setpoint = set;
	pid->automode = false;

	PID_Limits(pid, -255, 255);

	// Set default sample time to 25 ms
	pid->sampletime = 16 * (TICK_SECOND / 1000);

	PID_Direction(pid, E_PID_DIRECT);
	PID_Tune(pid, kp, ki, kd);

	pid->lasttime = millis() - pid->sampletime;

	return pid;
}


void PID_Compute(PID_t *pid)
{
	// Check if control is enabled
	if (!pid->automode)
		return;

	float in = *(pid->input);

	// Compute error
	float error = (*(pid->setpoint)) - in;

	// Compute integral
	pid->iterm += (pid->Ki * error);
	if (pid->iterm > pid->omax)
		pid->iterm = pid->omax;
	else if (pid->iterm < pid->omin)
		pid->iterm = pid->omin;

	// Compute differential on input
	float dinput = in - pid->lastin;

	// Compute PID output
	float out = pid->Kp * error + pid->iterm - pid->Kd * dinput;

	// Apply limit to output value
	if (out > pid->omax)
		out = pid->omax;
	else if (out < pid->omin)
		out = pid->omin;

	// Output to pointed variable
	(*pid->output) = out;

	// Keep track of some variables for next execution
	pid->lastin = in;
	pid->lasttime = millis();;
}


void PID_Tune(PID_t *pid, float kp, float ki, float kd)
{
	// Check for validity
	if (kp < 0 || ki < 0 || kd < 0)
		return;

	//Compute sample time in seconds
	float ssec = ((float) pid->sampletime) / ((float) TICK_SECOND);

	pid->Kp = kp;
	pid->Ki = ki * ssec;
	pid->Kd = kd / ssec;

	if (pid->direction == E_PID_REVERSE)
    {
		pid->Kp = 0 - pid->Kp;
		pid->Ki = 0 - pid->Ki;
		pid->Kd = 0 - pid->Kd;
	}
}


void PID_SampleTime(PID_t *pid, uint32_t time)
{
	if (time > 0)
    {
		float ratio = (float) (time * (TICK_SECOND / 1000)) / (float) pid->sampletime;
		pid->Ki *= ratio;
		pid->Kd /= ratio;
		pid->sampletime = time * (TICK_SECOND / 1000);
	}
}


void PID_Limits(PID_t *pid, float min, float max)
{
	if (min >= max)
	    return;

	pid->omin = min;
	pid->omax = max;

	//Adjust output to new limits
	if (pid->automode)
    {
		if (*(pid->output) > pid->omax)
			*(pid->output) = pid->omax;
		else if (*(pid->output) < pid->omin)
			*(pid->output) = pid->omin;

		if (pid->iterm > pid->omax)
			pid->iterm = pid->omax;
		else if (pid->iterm < pid->omin)
			pid->iterm = pid->omin;
	}
}


void PID_EnableAuto(PID_t *pid)
{
	// If going from manual to auto
	if (!pid->automode)
    {
		pid->iterm = *(pid->output);
		pid->lastin = *(pid->input);

		if (pid->iterm > pid->omax)
			pid->iterm = pid->omax;
		else if (pid->iterm < pid->omin)
			pid->iterm = pid->omin;

		pid->automode = true;
	}
}


void PID_Manual(PID_t *pid)
{
	pid->automode = false;
}


void PID_Direction(PID_t *pid, enum pid_control_directions dir)
{
	if (pid->automode && pid->direction != dir)
    {
		pid->Kp = (0 - pid->Kp);
		pid->Ki = (0 - pid->Ki);
		pid->Kd = (0 - pid->Kd);
	}
	pid->direction = dir;
}
