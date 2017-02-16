#include <assert.h>

#include "motor.h"

//===================================================================
// Memory Mapped Defines
//===================================================================
#define PWM_CW180 (*((unsigned short*) 0x80000010)) // u/d GPIO 40
#define PWM_CW360 (*((unsigned short*) 0x80000000)) // l/r GPIO 38
#define PWM_CWFIRE (*((unsigned short*) 0x80000060)) // fire GPIO 36

//===================================================================
// Motor Limits
//===================================================================
#define kCW180_MAX 7500
#define kCW180_MIN 1850
#define kCW180_MID (( kCW180_MIN + kCW180_MAX ) / 2)

#define kRANGE360 1000
#define kCW360_MID 4150
#define kCW360_MAX (kCW360_MID + kRANGE360)
#define kCW360_MIN (kCW360_MID - kRANGE360)

//===================================================================
// Constants
//===================================================================

//===================================================================
// Global variables
//===================================================================
unsigned short updown_speed;
unsigned short leftright_speed;

//===================================================================
// Private Function Definitions
//===================================================================
int motor_test()
{
	init_motors();
	while (1)
	{
		/*
		unsigned char c;
		printf("enter: ");
		scanf("%c", &c);
		if (c == 'w') move_up();
		else if (c == 'a') move_left();
		*/

		int pwm_counter;
		printf("enter: ");
		scanf("%d", &pwm_counter);

	}
	return 0;
}
//===================================================================
// Public Function Definitions
//===================================================================
void init_motors(void)
{
    set_motor_speed(1);
    PWM_CW180 = kCW180_MID;
    PWM_CW360 = kCW360_MID;
    PWM_CWFIRE = kCW180_MID;
}

void move_up(void)
{
    if (PWM_CW180 < kCW180_MAX)
    {
        PWM_CW180 += updown_speed;
        PWM_CW180 = PWM_CW180 < kCW180_MAX ? PWM_CW180 : kCW180_MAX;
        assert(PWM_CW180 <= kCW180_MAX);
    }
}

void move_down(void)
{
    if (PWM_CW180 > kCW180_MIN)
    {
        PWM_CW180 -= updown_speed;
        PWM_CW180 = PWM_CW180 > kCW180_MIN ? PWM_CW180 : kCW180_MIN;
        assert(PWM_CW180 >= kCW180_MIN);
    }
}

void move_left(void)
{
    if (PWM_CW360 < kCW360_MAX)
    {
        PWM_CW360 += leftright_speed;
        PWM_CW360 = PWM_CW360 < kCW360_MAX ? PWM_CW360 : kCW360_MAX;
        assert(PWM_CW360 <= kCW360_MAX);
    }
}

void move_right(void)
{
    if (PWM_CW360 > kCW360_MIN)
    {
        PWM_CW360 -= leftright_speed;
        PWM_CW360 = PWM_CW360 > kCW360_MIN ? PWM_CW360 : kCW360_MIN;
        assert(PWM_CW360 >= kCW360_MIN);
    }
}

void stop_leftrght(void)
{
    PWM_CW360 = kCW360_MID;
}

void motor_load(void)
{
    PWM_CWFIRE = kCW180_MID;
}

void motor_fire(void)
{
    PWM_CWFIRE = kCW180_MAX;
}

void set_motor_speed(int speed_multiplier)
{
    updown_speed = 50 * speed_multiplier;
    leftright_speed = 10 * speed_multiplier;
}

// Use this to debug!
void set_direct_PWM(int motor, int pwm_counter)
{
    if (motor == 0) PWM_CW180 = pwm_counter;
    else if (motor == 1) PWM_CW360 = pwm_counter;
    else if (motor == 2) PWM_CWFIRE = pwm_counter;
}
