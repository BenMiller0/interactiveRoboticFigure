#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <math.h>
#include <wiringPi.h>
#include <softPwm.h>

#define SAMPLE_RATE 48000
#define CHANNELS 1
#define FRAMES 1024
#define DEVICE_INPUT "plughw:CARD=Device,DEV=0"

#define SERVO_PIN 1  
#define SERVO_MIN_ANGLE 75  	// Minimum safe angle (closed mouth)
#define SERVO_MAX_ANGLE 100 	// Maximum safe angle (fully open mouth)
#define SERVO_MIN_PULSE 10  	// Pulse width for minimum angle (ms)
#define SERVO_MAX_PULSE 20  	// Pulse width for maximum angle (ms) - reduced range
#define SOUND_MIN_THRESHOLD 100 // Minimum sound level to trigger movement
#define SMOOTHING_FACTOR 0.8	// Higher = smoother but slower movement
#define DEAD_ZONE 300       	// Sound level below which we ignore input
#define SERVO_MOVEMENT_THRESHOLD 0.5 // Minimum angle change to move servo
#define SERVO_UPDATE_DELAY 20   // Delay between servo updates (ms)

#define MAX_SERVO_SPEED 5.0 	// Maximum degrees per update
#define SAFETY_MARGIN 2.0   	// Extra margin from physical limits

double prev_servo_angle = SERVO_MIN_ANGLE;  

void move_servo_based_on_amplitude(short *buffer, int size) {
	double sum = 0.0;
	for (int i = 0; i < size; i++) {
    	sum += abs(buffer[i]);
	}
	double avg_amplitude = sum / size;

	// Ignore quiet sounds
	if (avg_amplitude < SOUND_MIN_THRESHOLD + DEAD_ZONE) {
    	return;
	}

	// Calculate target angle with limits
	double target_angle = SERVO_MIN_ANGLE +
                     	((avg_amplitude / 32768.0) * (SERVO_MAX_ANGLE - SERVO_MIN_ANGLE));
    
	// Apply hard limits with safety margin
	target_angle = fmax(fmin(target_angle, SERVO_MAX_ANGLE - SAFETY_MARGIN),
                   	SERVO_MIN_ANGLE + SAFETY_MARGIN);

	// Smooth movement with speed limiting
	double angle_change = target_angle - prev_servo_angle;
	angle_change = fmax(fmin(angle_change, MAX_SERVO_SPEED), -MAX_SERVO_SPEED);
	double smoothed_angle = prev_servo_angle + angle_change * SMOOTHING_FACTOR;

	// Final safety check
	smoothed_angle = fmax(fmin(smoothed_angle, SERVO_MAX_ANGLE - SAFETY_MARGIN),
                     	SERVO_MIN_ANGLE + SAFETY_MARGIN);

	// Only move servo if change is significant
	if (fabs(smoothed_angle - prev_servo_angle) > SERVO_MOVEMENT_THRESHOLD) {
    	prev_servo_angle = smoothed_angle;
   	 
    	// Convert angle to pulse width with proper scaling
    	int pulse_width = SERVO_MIN_PULSE +
                     	((smoothed_angle - SERVO_MIN_ANGLE) *
                     	(SERVO_MAX_PULSE - SERVO_MIN_PULSE) /
                     	(SERVO_MAX_ANGLE - SERVO_MIN_ANGLE));

    	// Final pulse width limits
    	pulse_width = fmax(fmin(pulse_width, SERVO_MAX_PULSE), SERVO_MIN_PULSE);

    	softPwmWrite(SERVO_PIN, pulse_width);
    	delay(SERVO_UPDATE_DELAY);
	}
}

int main() {
	int err;
	snd_pcm_t *capture_handle;
	snd_pcm_hw_params_t *hw_params;
	short *buffer;

	if (wiringPiSetup() == -1) {
    	fprintf(stderr, "Failed to initialize WiringPi\n");
    	return 1;
	}

	pinMode(SERVO_PIN, PWM_OUTPUT);
	softPwmCreate(SERVO_PIN, SERVO_MIN_PULSE, 200);

	// Initialize servo to safe position
	softPwmWrite(SERVO_PIN, SERVO_MIN_PULSE);
	printf("Setting servo to minimum safe angle: %d degrees\n", SERVO_MIN_ANGLE);
	delay(500);

	if ((err = snd_pcm_open(&capture_handle, DEVICE_INPUT, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
    	fprintf(stderr, "Cannot open input device %s: %s\n", DEVICE_INPUT, snd_strerror(err));
    	return 1;
	}

	snd_pcm_hw_params_alloca(&hw_params);
	snd_pcm_hw_params_any(capture_handle, hw_params);
	snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(capture_handle, hw_params, SND_PCM_FORMAT_S16_LE);
	snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &(unsigned int){SAMPLE_RATE}, 0);
	snd_pcm_hw_params_set_channels(capture_handle, hw_params, CHANNELS);
	snd_pcm_hw_params(capture_handle, hw_params);

	buffer = (short *)malloc(FRAMES * CHANNELS * sizeof(short));

	printf("Animatronic mouth control active. Press Ctrl+C to stop.\n");
	printf("Servo range: %d-%d degrees\n", SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
	printf("Pulse width range: %d-%d ms\n", SERVO_MIN_PULSE, SERVO_MAX_PULSE);

	while (1) {
    	if ((err = snd_pcm_readi(capture_handle, buffer, FRAMES)) < 0) {
        	fprintf(stderr, "\nRead error: %s\n", snd_strerror(err));
        	snd_pcm_prepare(capture_handle);
    	}
    	move_servo_based_on_amplitude(buffer, FRAMES);
	}

	free(buffer);
	snd_pcm_close(capture_handle);
	return 0;
}
