/*
  SpindleControl.c - spindle control methods
  Part of Grbl-Advanced

  Copyright (c) 2012-2016 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2009-2011 Simen Svale Skogsrud
  Copyright (c)	2017 Patrick F.

  Grbl-Advanced is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl-Advanced is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl-Advanced.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "Protocol.h"
#include "Settings.h"
#include "System.h"
#include "GPIO.h"
#include "TIM.h"
#include "GCode.h"
#include "SpindleControl.h"
#include "Config.h"


static float pwm_gradient; // Precalulated value to speed up rpm to PWM conversions.
static uint8_t spindle_enabled = 0;


void Spindle_Init(void)
{
    // Configure variable spindle PWM and enable pin, if requried. On the Uno, PWM and enable are
    // combined unless configured otherwise.
    GPIO_InitGPIO(GPIO_SPINDLE);

    TIM1_Init();

    pwm_gradient = SPINDLE_PWM_RANGE/(settings.rpm_max-settings.rpm_min);

	Spindle_Stop();
}


// Disables the spindle and sets PWM output to zero when PWM variable spindle speed is enabled.
// Called by various main program and ISR routines. Keep routine small, fast, and efficient.
// Called by spindle_init(), spindle_set_speed(), spindle_set_state(), and mc_reset().
void Spindle_Stop(void)
{
	TIM1->CCR1 = 100; // Disable PWM. Output voltage is zero.
	spindle_enabled = 0;

#ifdef INVERT_SPINDLE_ENABLE_PIN
    GPIO_SetBits(GPIO_SPINDLE_ENA_PORT, GPIO_SPINDLE_ENA_PIN);
#else
    GPIO_ResetBits(GPIO_SPINDLE_ENA_PORT, GPIO_SPINDLE_ENA_PIN);
#endif
}


uint8_t Spindle_GetState(void)
{
    // Check if PWM is enabled.
	if(spindle_enabled)
    {
		/*if(SPINDLE_DIRECTION_PORT & (1<<SPINDLE_DIRECTION_BIT)) {
			return SPINDLE_STATE_CCW;
		}
		else {
			return SPINDLE_STATE_CW;
		}*/
		return SPINDLE_STATE_CW;
	}

	return SPINDLE_STATE_DISABLE;
}


// Sets spindle speed PWM output and enable pin, if configured. Called by spindle_set_state()
// and stepper ISR. Keep routine small and efficient.
void Spindle_SetSpeed(uint8_t pwm_value)
{
	TIM1->CCR1 = 100 - pwm_value; // Set PWM output level.
#ifdef SPINDLE_ENABLE_OFF_WITH_ZERO_SPEED
	if (pwm_value == SPINDLE_PWM_OFF_VALUE) {
		Spindle_Stop();
	}
	else {
		TIM_Cmd(TIM1, ENABLE); // Ensure PWM output is enabled.
  #ifdef INVERT_SPINDLE_ENABLE_PIN
		GPIO_ResetBits(GPIO_SPINDLE_ENA_PORT, GPIO_SPINDLE_ENA_PIN);
  #else
		GPIO_SetBits(GPIO_SPINDLE_ENA_PORT, GPIO_SPINDLE_ENA_PIN);
  #endif
        spindle_enabled = 1;
	}
#else
	if(pwm_value == SPINDLE_PWM_OFF_VALUE) {
		TIM_Cmd(TIM1, DISABLE); // Disable PWM. Output voltage is zero.
		spindle_enabled = 0;
	}
	else {
		TIM_Cmd(TIM1, ENABLE); // Ensure PWM output is enabled.
		spindle_enabled = 1;
	}
#endif
}


// Called by spindle_set_state() and step segment generator. Keep routine small and efficient.
uint8_t Spindle_ComputePwmValue(float rpm) // 328p PWM register is 8-bit.
{
	uint8_t pwm_value;

	rpm *= (0.010*sys.spindle_speed_ovr); // Scale by spindle speed override value.

	// Calculate PWM register value based on rpm max/min settings and programmed rpm.
	if((settings.rpm_min >= settings.rpm_max) || (rpm >= settings.rpm_max)) {
		// No PWM range possible. Set simple on/off spindle control pin state.
		sys.spindle_speed = settings.rpm_max;
		pwm_value = SPINDLE_PWM_MAX_VALUE;
	}
	else if(rpm <= settings.rpm_min) {
		if(rpm == 0.0) { // S0 disables spindle
			sys.spindle_speed = 0.0;
			pwm_value = SPINDLE_PWM_OFF_VALUE;
		}
		else { // Set minimum PWM output
			sys.spindle_speed = settings.rpm_min;
			pwm_value = SPINDLE_PWM_MIN_VALUE;
		}
	}
	else {
		// Compute intermediate PWM value with linear spindle speed model.
		// NOTE: A nonlinear model could be installed here, if required, but keep it VERY light-weight.
		sys.spindle_speed = rpm;
		pwm_value = floor((rpm-settings.rpm_min)*pwm_gradient) + SPINDLE_PWM_MIN_VALUE;
	}

	return pwm_value;
}


// Immediately sets spindle running state with direction and spindle rpm via PWM, if enabled.
// Called by g-code parser spindle_sync(), parking retract and restore, g-code program end,
// sleep, and spindle stop override.
void Spindle_SetState(uint8_t state, float rpm)
{
	if(sys.abort) {
		// Block during abort.
		return;
	}

	if(state == SPINDLE_DISABLE) { // Halt or set spindle direction and rpm.
		sys.spindle_speed = 0.0;
		Spindle_Stop();
	}
	else {
		if(state == SPINDLE_ENABLE_CW) {
			GPIO_ResetBits(GPIO_SPINDLE_DIR_PORT, GPIO_SPINDLE_DIR_PIN);
		}
		else {
			GPIO_SetBits(GPIO_SPINDLE_DIR_PORT, GPIO_SPINDLE_DIR_PIN);
		}

		// NOTE: Assumes all calls to this function is when Grbl is not moving or must remain off.
		if(settings.flags & BITFLAG_LASER_MODE) {
			if(state == SPINDLE_ENABLE_CCW) {
				// TODO: May need to be rpm_min*(100/MAX_SPINDLE_SPEED_OVERRIDE);
				rpm = 0.0;
			}
		}

		Spindle_SetSpeed(Spindle_ComputePwmValue(rpm));
	}

	sys.report_ovr_counter = 0; // Set to report change immediately
}


// G-code parser entry-point for setting spindle state. Forces a planner buffer sync and bails
// if an abort or check-mode is active.
void Spindle_Sync(uint8_t state, float rpm)
{
	if(sys.state == STATE_CHECK_MODE) {
		return;
	}

	Protocol_BufferSynchronize(); // Empty planner buffer to ensure spindle is set when programmed.
	Spindle_SetState(state, rpm);
}

