#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>

#define GPIO_PATH_EXPORT    "/sys/class/gpio/export"
#define GPIO_PATH_UNEXPORT  "/sys/class/gpio/unexport"
#define GPIO_PATH_DIRECTION "/sys/class/gpio/gpio%d/direction"
#define GPIO_PATH_VALUE     "/sys/class/gpio/gpio%d/value"

#define BUTTON_GPIO_PIN_1 (uint8_t)48
#define RED_LED_GPIO_PIN_1 (uint8_t)49
#define GREEN_LED_GPIO_PIN_1 (uint8_t)117
#define YELLOW_LED_GPIO_PIN_1 (uint8_t)115

#define BUTTON_GPIO_PIN_2 (uint8_t)44
#define RED_LED_GPIO_PIN_2 (uint8_t)26
#define GREEN_LED_GPIO_PIN_2 (uint8_t)46
#define YELLOW_LED_GPIO_PIN_2 (uint8_t)65

pthread_mutex_t mx;
sem_t sem_button1;
sem_t sem_button2;

uint8_t s1 = 0;
uint8_t s2 = 1;

void set_gpio_value(uint8_t gpio_pin, uint8_t value) {
    char path[64];
    if (snprintf(path, sizeof(path), GPIO_PATH_VALUE, gpio_pin) >= 0) {
        FILE* value_file = fopen(path, "w");
        if (value_file) {
            fprintf(value_file, "%d", value);
            fclose(value_file);
        }
    }
}

void set_gpio_direction(uint8_t gpio_pin, const char* direction) {
    char path[64];
    if (snprintf(path, sizeof(path), GPIO_PATH_DIRECTION, gpio_pin) >= 0) {
        FILE* direction_file = fopen(path, "w");
        if (direction_file) {
            fprintf(direction_file, "%s", direction);
            fclose(direction_file);
        }
    }
}

void unexport_gpio(uint8_t gpio_pin) {
    FILE* file2 = fopen(GPIO_PATH_UNEXPORT, "w");
    if (file2) {
        fprintf(file2, "%d", gpio_pin);
        fclose(file2);
    }
}

void export_gpio(uint8_t gpio_pin) {
    FILE* file1 = fopen(GPIO_PATH_EXPORT, "w");
    if (file1) {
        fprintf(file1, "%d", gpio_pin);
        fclose(file1);
    }
}

uint8_t read_button(uint8_t gpio_pin) {
    char path[64];
    if (snprintf(path, sizeof(path), GPIO_PATH_VALUE, gpio_pin) >= 0) {
        FILE* value_file = fopen(path, "r");
        if (value_file) {
            char buffer[2];
            if (fgets(buffer, sizeof(buffer), value_file) != NULL) {
                fclose(value_file);
                return (uint8_t)atoi(buffer);
            }
            fclose(value_file);
        }
    }
    return (uint8_t)0xFF;
}

void* thread1(void* arg) {
    uint8_t button_pressed = 0;
    struct timespec press_time, release_time;
    while (1) {
        button_pressed = read_button(BUTTON_GPIO_PIN_1);
        if (button_pressed == (uint8_t)1) {
            clock_gettime(CLOCK_REALTIME, &press_time);
            usleep((useconds_t)100000);
            button_pressed = read_button(BUTTON_GPIO_PIN_1);
            if (button_pressed == (uint8_t)1) {
                sem_wait(&sem_button1);
                pthread_mutex_lock(&mx);
                clock_gettime(CLOCK_REALTIME, &release_time);
                while (button_pressed == (uint8_t)1) {
                    if (release_time.tv_sec - press_time.tv_sec >= 5) {
                        set_gpio_value(RED_LED_GPIO_PIN_1, 0);
                        set_gpio_value(GREEN_LED_GPIO_PIN_1, 1);
                        set_gpio_value(YELLOW_LED_GPIO_PIN_1, 0);

                        set_gpio_value(RED_LED_GPIO_PIN_2, 1);
                        set_gpio_value(GREEN_LED_GPIO_PIN_2, 0);
                        set_gpio_value(YELLOW_LED_GPIO_PIN_2, 0);
                        
                        s1 = 1;
                        s2 = 0;

                        usleep(4000000);

                        pthread_mutex_unlock(&mx);
                        sem_post(&sem_button1);
                        return NULL;
                    }
                    usleep((useconds_t)100000);
                    button_pressed = read_button(BUTTON_GPIO_PIN_1);
                    clock_gettime(CLOCK_REALTIME, &release_time);
                }
                pthread_mutex_unlock(&mx);
                sem_post(&sem_button1);
            }
        }
        usleep((useconds_t)100000);
    }
    return NULL;
}

void* thread2(void* arg) {
    uint8_t button_pressed = 0;
    struct timespec press_time, release_time;
    while (1) {
        button_pressed = read_button(BUTTON_GPIO_PIN_2);
        if (button_pressed == (uint8_t)1) {
            clock_gettime(CLOCK_REALTIME, &press_time);
            usleep((useconds_t)100000);
            button_pressed = read_button(BUTTON_GPIO_PIN_2);
            if (button_pressed == (uint8_t)1) {
                sem_wait(&sem_button2);
                pthread_mutex_lock(&mx);
                clock_gettime(CLOCK_REALTIME, &release_time);
                while (button_pressed == (uint8_t)1) {
                    if (release_time.tv_sec - press_time.tv_sec >= 5) {
                        set_gpio_value(RED_LED_GPIO_PIN_2, 0);
                        set_gpio_value(GREEN_LED_GPIO_PIN_2, 1);
                        set_gpio_value(YELLOW_LED_GPIO_PIN_2, 0);

                        set_gpio_value(RED_LED_GPIO_PIN_1, 1);
                        set_gpio_value(GREEN_LED_GPIO_PIN_1, 0);
                        set_gpio_value(YELLOW_LED_GPIO_PIN_1, 0);

                        s1 = 0;
                        s2 = 1;

                        usleep(4000000);


                        pthread_mutex_unlock(&mx);
                        sem_post(&sem_button2);
                        return NULL;
                    }
                    usleep((useconds_t)100000);
                    button_pressed = read_button(BUTTON_GPIO_PIN_2);
                    clock_gettime(CLOCK_REALTIME, &release_time);
                }
                pthread_mutex_unlock(&mx);
                sem_post(&sem_button2);
            }
        }
        usleep((useconds_t)100000);
    }
    return NULL;
}

void* traffic_lights1(void* arg) {
    uint8_t s1 = 0;
    const uint8_t duration_of_light[] = {15, 10, 5};
    while (1) {
        pthread_mutex_lock(&mx);

        if (s1 == 0) {
            set_gpio_value(RED_LED_GPIO_PIN_1, 1);
            set_gpio_value(GREEN_LED_GPIO_PIN_1, 0);
            set_gpio_value(YELLOW_LED_GPIO_PIN_1, 0);
        } else if (s1 == 1) {
            set_gpio_value(RED_LED_GPIO_PIN_1, 0);
            set_gpio_value(GREEN_LED_GPIO_PIN_1, 1);
            set_gpio_value(YELLOW_LED_GPIO_PIN_1, 0);
        } else if (s1 == 2) {
            set_gpio_value(RED_LED_GPIO_PIN_1, 0);
            set_gpio_value(GREEN_LED_GPIO_PIN_1, 0);
            set_gpio_value(YELLOW_LED_GPIO_PIN_1, 1);
        }

        pthread_mutex_unlock(&mx);

        sleep((unsigned int)duration_of_light[s1]);

        s1 = (uint8_t)((s1 + 1) % 3);
    }
    return NULL;
}

void* traffic_lights2(void* arg) {
    uint8_t s2 = 1;
    const uint8_t duration_of_light[] = {15, 10, 5};

    while (1) {
        pthread_mutex_lock(&mx);

        if (s2 == 0) {
            set_gpio_value(RED_LED_GPIO_PIN_2, 1);
            set_gpio_value(GREEN_LED_GPIO_PIN_2, 0);
            set_gpio_value(YELLOW_LED_GPIO_PIN_2, 0);
        } else if (s2 == 1) {
            set_gpio_value(RED_LED_GPIO_PIN_2, 0);
            set_gpio_value(GREEN_LED_GPIO_PIN_2, 1);
            set_gpio_value(YELLOW_LED_GPIO_PIN_2, 0);
        } else if (s2 == 2) {
            set_gpio_value(RED_LED_GPIO_PIN_2, 0);
            set_gpio_value(GREEN_LED_GPIO_PIN_2, 0);
            set_gpio_value(YELLOW_LED_GPIO_PIN_2, 1);
        }

        pthread_mutex_unlock(&mx);

        sleep((unsigned int)duration_of_light[s2]);

        s2 = (uint8_t)((s2 + 1) % 3);

    }
    return NULL;
}

int main(void) {
    pthread_mutex_init(&mx, NULL);
    sem_init(&sem_button1, 0, 1);
    sem_init(&sem_button2, 0, 1);

    export_gpio(BUTTON_GPIO_PIN_1);
    export_gpio(RED_LED_GPIO_PIN_1);
    export_gpio(GREEN_LED_GPIO_PIN_1);
    export_gpio(YELLOW_LED_GPIO_PIN_1);

    set_gpio_direction(BUTTON_GPIO_PIN_1, "in");
    set_gpio_direction(RED_LED_GPIO_PIN_1, "out");
    set_gpio_direction(GREEN_LED_GPIO_PIN_1, "out");
    set_gpio_direction(YELLOW_LED_GPIO_PIN_1, "out");

    export_gpio(BUTTON_GPIO_PIN_2);
    export_gpio(RED_LED_GPIO_PIN_2);
    export_gpio(GREEN_LED_GPIO_PIN_2);
    export_gpio(YELLOW_LED_GPIO_PIN_2);

    set_gpio_direction(BUTTON_GPIO_PIN_2, "in");
    set_gpio_direction(RED_LED_GPIO_PIN_2, "out");
    set_gpio_direction(GREEN_LED_GPIO_PIN_2, "out");
    set_gpio_direction(YELLOW_LED_GPIO_PIN_2, "out");

    pthread_t set_thread1;
    pthread_t set_thread2;
    pthread_t button_thread1;
    pthread_t button_thread2;

    pthread_create(&set_thread1, NULL, traffic_lights1, NULL);
    pthread_create(&set_thread2, NULL, traffic_lights2, NULL);
    pthread_create(&button_thread1, NULL, thread1, NULL);
    pthread_create(&button_thread2, NULL, thread2, NULL);

    pthread_join(set_thread1, NULL);
    pthread_join(set_thread2, NULL);
    pthread_join(button_thread1, NULL);
    pthread_join(button_thread2, NULL);

    unexport_gpio(BUTTON_GPIO_PIN_1);
    unexport_gpio(RED_LED_GPIO_PIN_1);
    unexport_gpio(GREEN_LED_GPIO_PIN_1);
    unexport_gpio(YELLOW_LED_GPIO_PIN_1);

    unexport_gpio(BUTTON_GPIO_PIN_2);
    unexport_gpio(RED_LED_GPIO_PIN_2);
    unexport_gpio(GREEN_LED_GPIO_PIN_2);
    unexport_gpio(YELLOW_LED_GPIO_PIN_2);

    pthread_mutex_destroy(&mx);
    sem_destroy(&sem_button1);
    sem_destroy(&sem_button2);

    return 0;
}

