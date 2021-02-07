// Original implementation: https://github.com/geekfactory/PID
#ifndef PID_H_INCLUDED
#define PID_H_INCLUDED


#include <stdbool.h>
#include <stdint.h>


/**
 * Defines if the controler is direct or reverse
 */
enum pid_control_directions
{
    E_PID_DIRECT,
    E_PID_REVERSE,
};

/**
 * Structure that holds PID all the PID controller data, multiple instances are
 * posible using different structures for each controller
 */
typedef struct
{
    // Input, output and setpoint
    float *input; //!< Current Process Value
    float *output; //!< Corrective Output from PID Controller
    float *setpoint; //!< Controller Setpoint

    // Tuning parameters
    float Kp; //!< Stores the gain for the Proportional term
    float Ki; //!< Stores the gain for the Integral term
    float Kd; //!< Stores the gain for the Derivative term

    // Output minimum and maximum values
    float omin; //!< Maximum value allowed at the output
    float omax; //!< Minimum value allowed at the output

    // Variables for PID algorithm
    float iterm; //!< Accumulator for integral term
    float lastin; //!< Last input value for differential term

    // Time related
    uint32_t lasttime; //!< Stores the time when the control loop ran last time
    uint32_t sampletime; //!< Defines the PID sample time

    // Operation mode
    uint8_t automode; //!< Defines if the PID controller is enabled or disabled
    enum pid_control_directions direction;
} PID_t;


#ifdef	__cplusplus
extern "C" {
#endif


/**
 * @brief Creates a new PID controller
 *
 * Creates a new pid controller and initializes it�s input, output and internal
 * variables. Also we set the tuning parameters
 *
 * @param pid A pointer to a pid_controller structure
 * @param in Pointer to float value for the process input
 * @param out Poiter to put the controller output value
 * @param set Pointer float with the process setpoint value
 * @param kp Proportional gain
 * @param ki Integral gain
 * @param kd Diferential gain
 *
 * @return returns a pid_t controller handle
 */
PID_t *PID_Create(PID_t *pid, float* in, float* out, float* set, float kp, float ki, float kd);

/**
 * @brief Computes the output of the PID control
 *
 * This function computes the PID output based on the parameters, setpoint and
 * current system input.
 *
 * @param pid The PID controller instance which will be used for computation
 */
void PID_Compute(PID_t *pid);

/**
 * @brief Sets new PID tuning parameters
 *
 * Sets the gain for the Proportional (Kp), Integral (Ki) and Derivative (Kd)
 * terms.
 *
 * @param pid The PID controller instance to modify
 * @param kp Proportional gain
 * @param ki Integral gain
 * @param kd Derivative gain
 */
void PID_Tune(PID_t *pid, float kp, float ki, float kd);

/**
 * @brief Sets the pid algorithm period
 *
 * Changes the between PID control loop computations.
 *
 * @param pid The PID controller instance to modify
 * @param time The time in milliseconds between computations
 */
void PID_SampleTime(PID_t *pid, uint32_t time);

/**
 * @brief Sets the limits for the PID controller output
 *
 * @param pid The PID controller instance to modify
 * @param min The minimum output value for the PID controller
 * @param max The maximum output value for the PID controller
 */
void PID_Limits(PID_t *pid, float min, float max);

/**
 * @brief Enables automatic control using PID
 *
 * Enables the PID control loop. If manual output adjustment is needed you can
 * disable the PID control loop using pid_manual(). This function enables PID
 * automatic control at program start or after calling pid_manual()
 *
 * @param pid The PID controller instance to enable
 */
void PID_EnableAuto(PID_t *pid);

/**
 * @brief Disables automatic process control
 *
 * Disables the PID control loop. User can modify the value of the output
 * variable and the controller will not overwrite it.
 *
 * @param pid The PID controller instance to disable
 */
void PID_Manual(PID_t *pid);

/**
 * @brief Configures the PID controller direction
 *
 * Sets the direction of the PID controller. The direction is "DIRECT" when a
 * increase of the output will cause a increase on the measured value and
 * "REVERSE" when a increase on the controller output will cause a decrease on
 * the measured value.
 *
 * @param pid The PID controller instance to modify
 * @param direction The new direction of the PID controller
 */
void PID_Direction(PID_t *pid, enum pid_control_directions dir);


#ifdef	__cplusplus
}
#endif


#endif /* PID_H_INCLUDED */
