#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define CONFIG_PATH "/root/fancontrol/fansettings.config"
#define TEMP_PATH "/sys/class/thermal/thermal_zone0/temp"
#define PWM_CHIP_PATH "/sys/class/pwm/pwmchip1"
#define PWM_ENABLE_PATH PWM_CHIP_PATH "/pwm0/enable"
#define PWM_PERIOD_PATH PWM_CHIP_PATH "/pwm0/period"
#define PWM_DUTY_PATH PWM_CHIP_PATH "/pwm0/duty_cycle"
#define PWM_EXPORT_PATH PWM_CHIP_PATH "/export"
#define LOG_PATH "/root/fancontrol/fancontrol.log"

#define MAX_THRESHOLDS 12

int temp_thresholds[MAX_THRESHOLDS] = {40, 50, 60, 70, 80, 85, 90};
int pwm_values[MAX_THRESHOLDS] = {50000, 40000, 30000, 20000, 15000, 10000, 5000};  // Added new values for higher temperatures
int num_thresholds = 7;  // Adjusted to include the new thresholds
int pwm_period = 50000;
int last_duty_cycle = -1; // Track last set duty cycle

void log_message(const char *message) {
    FILE *log_file = fopen(LOG_PATH, "a");
    if (!log_file) return;

    // Limit log file to 40 lines
    char buffer[1024];
    FILE *temp_log = fopen(LOG_PATH, "r");
    int line_count = 0;

    if (temp_log) {
        while (fgets(buffer, sizeof(buffer), temp_log)) {
            line_count++;
        }
        fclose(temp_log);
    }

    if (line_count >= 40) {
        temp_log = fopen(LOG_PATH, "w");
        if (temp_log) {
            fprintf(temp_log, "Log reset to avoid overflow.\n");
            fclose(temp_log);
        }
    }

    fprintf(log_file, "%s\n", message);
    fclose(log_file);
}

void load_config() {
    FILE *file = fopen(CONFIG_PATH, "r");
    if (!file) {
        log_message("Config file not found, using defaults.");
        return;
    }

    char line[256];
    num_thresholds = 0;

    while (fgets(line, sizeof(line), file) && num_thresholds < MAX_THRESHOLDS) {
        int temp, pwm;
        if (sscanf(line, "temp_%d = %d", &temp, &pwm) == 2) {
            temp_thresholds[num_thresholds] = temp;
            pwm_values[num_thresholds] = pwm;
            num_thresholds++;
        } else if (sscanf(line, "pwm_period = %d", &pwm_period) == 1) {
            continue;
        }
    }

    fclose(file);

    char log_buf[128];
    snprintf(log_buf, sizeof(log_buf), "Loaded %d temperature levels.", num_thresholds);
    log_message(log_buf);
}

int read_temperature() {
    FILE *file = fopen(TEMP_PATH, "r");
    if (!file) return -1;

    int temp;
    fscanf(file, "%d", &temp);
    fclose(file);
    
    return temp / 1000; // Convert millidegrees to degrees
}

int is_pwm_exported() {
    FILE *enable_file = fopen(PWM_ENABLE_PATH, "r");
    if (enable_file) {
        fclose(enable_file);
        return 1; // PWM already exported
    }
    return 0;
}

void set_pwm(int duty_cycle) {
    if (duty_cycle == last_duty_cycle) return; // Skip if already set

    char cmd[128];

    if (!is_pwm_exported()) {
        snprintf(cmd, sizeof(cmd), "echo 0 > %s", PWM_EXPORT_PATH);
        int ret = system(cmd);
        if (ret != 0) {
            log_message("Failed to export PWM.");
            return;
        }
    }

    snprintf(cmd, sizeof(cmd), "echo %d > %s", pwm_period, PWM_PERIOD_PATH);
    if (system(cmd) != 0) {
        log_message("Failed to set PWM period.");
        return;
    }

    snprintf(cmd, sizeof(cmd), "echo %d > %s", duty_cycle, PWM_DUTY_PATH);
    if (system(cmd) != 0) {
        log_message("Failed to set PWM duty cycle.");
        return;
    }

    snprintf(cmd, sizeof(cmd), "echo 1 > %s", PWM_ENABLE_PATH);
    if (system(cmd) != 0) {
        log_message("Failed to enable PWM.");
        return;
    }

    last_duty_cycle = duty_cycle;

    char log_buf[64];
    snprintf(log_buf, sizeof(log_buf), "Temperature: %d, PWM Duty: %d", read_temperature(), duty_cycle);
    log_message(log_buf);
}

void control_fan() {
    int temp = read_temperature();
    if (temp < 0) return;

    int duty_cycle = pwm_values[0];

    for (int i = 0; i < num_thresholds; i++) {
        if (temp >= temp_thresholds[i]) {
            duty_cycle = pwm_values[i];
        } else {
            break;
        }
    }

    set_pwm(duty_cycle);
}

int main() {
    load_config(); // Load config on startup

    while (1) {
        control_fan();
        sleep(5);
    }
    return 0;
}
