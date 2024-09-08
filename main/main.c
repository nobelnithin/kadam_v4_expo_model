#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "driver/i2c.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_sleep.h"

#define BTN1 GPIO_NUM_19
#define BTN2 GPIO_NUM_21
#define BTN3 GPIO_NUM_18
#define VBUS_INTR GPIO_NUM_42

#define SEG_A_GPIO 12
#define SEG_B_GPIO 13
#define SEG_C_GPIO 35
#define SEG_D_GPIO 33
#define SEG_E_GPIO 34
#define SEG_F_GPIO 10
#define SEG_G_GPIO 11

#define LED_BLUE1 3
#define LED_BLUE2 4
#define RGB_RED 15
#define RGB_GREEN 16
#define RGB_BLUE 17

xQueueHandle BTN1Queue;
xQueueHandle BTN2Queue;
xQueueHandle BTN3Queue;
xQueueHandle VBUS_INTRQueue;

bool isLed_menu = true;
bool isLed_setup = false;
bool isLed_walk = false;   
bool long_press_detected = false;

uint64_t btn1pre_time = 0;
uint64_t btn1intr_time = 0;
uint64_t btn1curr_time = 0;

uint64_t btn2pre_time = 0;
uint64_t btn2intr_time = 0;
uint64_t btn2curr_time = 0;
uint64_t pre_time_vbus_intr = 0;

uint64_t btn3pre_time = 0;
uint64_t btn3intr_time = 0;
uint64_t btn3curr_time = 0;

int menu_cursor=0;
int setup_cursor=0;

bool vbus_flag =true;

void enter_deep_sleep() {
    ESP_LOGI("NO TAG", "Entering deep sleep in 500ms");
    vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 500ms to ensure button is released
    esp_sleep_enable_ext0_wakeup(BTN2, 0); // Wake up when button is pressed (falling edge)
    esp_deep_sleep_start();
}


static void led_menu()
{
    isLed_menu = true;
    isLed_setup = false;
    isLed_walk = false;
    gpio_set_direction(SEG_A_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_B_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_C_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_D_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_E_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_F_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_G_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_BLUE1, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_BLUE2, GPIO_MODE_OUTPUT);
    gpio_set_level(SEG_A_GPIO, 0);
    gpio_set_level(SEG_B_GPIO, 0);
    gpio_set_level(SEG_C_GPIO, 0);
    gpio_set_level(SEG_D_GPIO, 0);
    gpio_set_level(SEG_E_GPIO, 0);
    gpio_set_level(SEG_F_GPIO, 0);
    gpio_set_level(SEG_G_GPIO, 1);
    if(menu_cursor==0)
    {
        gpio_set_level(LED_BLUE2, 1);
        // gpio_set_level(LED_BLUE1, 0);
    }
    else if(menu_cursor == 1)
    {
        gpio_set_level(LED_BLUE1, 1);
        // gpio_set_level(LED_BLUE2, 0);
    }
    gpio_set_direction(RGB_GREEN,GPIO_MODE_OUTPUT);
    gpio_set_level(RGB_GREEN,0);

    printf("menu cursor %d\n",menu_cursor);
}

static void display_number(int number)
{
    // Set segments based on the number to display
    switch (number)
    {
        case 0: // Display '0'
            gpio_set_level(SEG_A_GPIO, 1);
            gpio_set_level(SEG_B_GPIO, 1);
            gpio_set_level(SEG_C_GPIO, 1);
            gpio_set_level(SEG_D_GPIO, 1);
            gpio_set_level(SEG_E_GPIO, 1);
            gpio_set_level(SEG_F_GPIO, 1);
            gpio_set_level(SEG_G_GPIO, 0);
            break;
        case 1: // Display '1'
            gpio_set_level(SEG_A_GPIO, 0);
            gpio_set_level(SEG_B_GPIO, 1);
            gpio_set_level(SEG_C_GPIO, 1);
            gpio_set_level(SEG_D_GPIO, 0);
            gpio_set_level(SEG_E_GPIO, 0);
            gpio_set_level(SEG_F_GPIO, 0);
            gpio_set_level(SEG_G_GPIO, 0);
            break;
        case 2: // Display '2'
            gpio_set_level(SEG_A_GPIO, 1);
            gpio_set_level(SEG_B_GPIO, 1);
            gpio_set_level(SEG_C_GPIO, 0);
            gpio_set_level(SEG_D_GPIO, 1);
            gpio_set_level(SEG_E_GPIO, 1);
            gpio_set_level(SEG_F_GPIO, 0);
            gpio_set_level(SEG_G_GPIO, 1);
            break;
        case 3: // Display '3'
            gpio_set_level(SEG_A_GPIO, 1);
            gpio_set_level(SEG_B_GPIO, 1);
            gpio_set_level(SEG_C_GPIO, 1);
            gpio_set_level(SEG_D_GPIO, 1);
            gpio_set_level(SEG_E_GPIO, 0);
            gpio_set_level(SEG_F_GPIO, 0);
            gpio_set_level(SEG_G_GPIO, 1);
            break;
        case 4: // Display '4'
            gpio_set_level(SEG_A_GPIO, 0);
            gpio_set_level(SEG_B_GPIO, 1);
            gpio_set_level(SEG_C_GPIO, 1);
            gpio_set_level(SEG_D_GPIO, 0);
            gpio_set_level(SEG_E_GPIO, 0);
            gpio_set_level(SEG_F_GPIO, 1);
            gpio_set_level(SEG_G_GPIO, 1);
            break;
        case 5: // Display '5'
            gpio_set_level(SEG_A_GPIO, 1);
            gpio_set_level(SEG_B_GPIO, 0);
            gpio_set_level(SEG_C_GPIO, 1);
            gpio_set_level(SEG_D_GPIO, 1);
            gpio_set_level(SEG_E_GPIO, 0);
            gpio_set_level(SEG_F_GPIO, 1);
            gpio_set_level(SEG_G_GPIO, 1);
            break;
        case 6: // Display '6'
            gpio_set_level(SEG_A_GPIO, 1);
            gpio_set_level(SEG_B_GPIO, 0);
            gpio_set_level(SEG_C_GPIO, 1);
            gpio_set_level(SEG_D_GPIO, 1);
            gpio_set_level(SEG_E_GPIO, 1);
            gpio_set_level(SEG_F_GPIO, 1);
            gpio_set_level(SEG_G_GPIO, 1);
            break;
        case 7: // Display '7'
            gpio_set_level(SEG_A_GPIO, 1);
            gpio_set_level(SEG_B_GPIO, 1);
            gpio_set_level(SEG_C_GPIO, 1);
            gpio_set_level(SEG_D_GPIO, 0);
            gpio_set_level(SEG_E_GPIO, 0);
            gpio_set_level(SEG_F_GPIO, 0);
            gpio_set_level(SEG_G_GPIO, 0);
            break;
        case 8: // Display '8'
            gpio_set_level(SEG_A_GPIO, 1);
            gpio_set_level(SEG_B_GPIO, 1);
            gpio_set_level(SEG_C_GPIO, 1);
            gpio_set_level(SEG_D_GPIO, 1);
            gpio_set_level(SEG_E_GPIO, 1);
            gpio_set_level(SEG_F_GPIO, 1);
            gpio_set_level(SEG_G_GPIO, 1);
            break;
        case 9: // Display '9'
            gpio_set_level(SEG_A_GPIO, 1);
            gpio_set_level(SEG_B_GPIO, 1);
            gpio_set_level(SEG_C_GPIO, 1);
            gpio_set_level(SEG_D_GPIO, 1);
            gpio_set_level(SEG_E_GPIO, 0);
            gpio_set_level(SEG_F_GPIO, 1);
            gpio_set_level(SEG_G_GPIO, 1);
            break;

        default:
            printf("Invalid setup_cursor value\n");
            break;
    }
}

static void turn_off_segments()
{
    gpio_set_level(SEG_A_GPIO, 0);
    gpio_set_level(SEG_B_GPIO, 0);
    gpio_set_level(SEG_C_GPIO, 0);
    gpio_set_level(SEG_D_GPIO, 0);
    gpio_set_level(SEG_E_GPIO, 0);
    gpio_set_level(SEG_F_GPIO, 0);
    gpio_set_level(SEG_G_GPIO, 0);
}


static void led_setup()
{
    isLed_menu = false;
    isLed_setup = true;
    isLed_walk = false;
    // Set GPIO direction for seven-segment display
    gpio_set_direction(SEG_A_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_B_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_C_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_D_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_E_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_F_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_G_GPIO, GPIO_MODE_OUTPUT);
                gpio_set_direction(RGB_GREEN,GPIO_MODE_OUTPUT);
                gpio_set_level(RGB_GREEN,1);


    printf("Setup cursor %d\n", setup_cursor);

}



static void led_walk()
{
    isLed_menu = false;
    isLed_setup = false;
    isLed_walk = true;
    gpio_set_direction(SEG_A_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_B_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_C_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_D_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_E_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_F_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_G_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_BLUE2, GPIO_MODE_OUTPUT);
    printf("setup cursor %d\n",setup_cursor);
                gpio_set_direction(RGB_GREEN,GPIO_MODE_OUTPUT);
                gpio_set_level(RGB_GREEN,1);
    gpio_set_level(LED_BLUE2, 0);
    switch (setup_cursor)
    {
        case 0: // Display '0'
            gpio_set_level(SEG_A_GPIO, 1);
            gpio_set_level(SEG_B_GPIO, 1);
            gpio_set_level(SEG_C_GPIO, 1);
            gpio_set_level(SEG_D_GPIO, 1);
            gpio_set_level(SEG_E_GPIO, 1);
            gpio_set_level(SEG_F_GPIO, 1);
            gpio_set_level(SEG_G_GPIO, 0);
            break;
        case 1: // Display '1'
            gpio_set_level(SEG_A_GPIO, 0);
            gpio_set_level(SEG_B_GPIO, 1);
            gpio_set_level(SEG_C_GPIO, 1);
            gpio_set_level(SEG_D_GPIO, 0);
            gpio_set_level(SEG_E_GPIO, 0);
            gpio_set_level(SEG_F_GPIO, 0);
            gpio_set_level(SEG_G_GPIO, 0);
            break;
        case 2: // Display '2'
            gpio_set_level(SEG_A_GPIO, 1);
            gpio_set_level(SEG_B_GPIO, 1);
            gpio_set_level(SEG_C_GPIO, 0);
            gpio_set_level(SEG_D_GPIO, 1);
            gpio_set_level(SEG_E_GPIO, 1);
            gpio_set_level(SEG_F_GPIO, 0);
            gpio_set_level(SEG_G_GPIO, 1);
            break;
        case 3: // Display '3'
            gpio_set_level(SEG_A_GPIO, 1);
            gpio_set_level(SEG_B_GPIO, 1);
            gpio_set_level(SEG_C_GPIO, 1);
            gpio_set_level(SEG_D_GPIO, 1);
            gpio_set_level(SEG_E_GPIO, 0);
            gpio_set_level(SEG_F_GPIO, 0);
            gpio_set_level(SEG_G_GPIO, 1);
            break;
        case 4: // Display '4'
            gpio_set_level(SEG_A_GPIO, 0);
            gpio_set_level(SEG_B_GPIO, 1);
            gpio_set_level(SEG_C_GPIO, 1);
            gpio_set_level(SEG_D_GPIO, 0);
            gpio_set_level(SEG_E_GPIO, 0);
            gpio_set_level(SEG_F_GPIO, 1);
            gpio_set_level(SEG_G_GPIO, 1);
            break;
        case 5: // Display '5'
            gpio_set_level(SEG_A_GPIO, 1);
            gpio_set_level(SEG_B_GPIO, 0);
            gpio_set_level(SEG_C_GPIO, 1);
            gpio_set_level(SEG_D_GPIO, 1);
            gpio_set_level(SEG_E_GPIO, 0);
            gpio_set_level(SEG_F_GPIO, 1);
            gpio_set_level(SEG_G_GPIO, 1);
            break;
        case 6: // Display '6'
            gpio_set_level(SEG_A_GPIO, 1);
            gpio_set_level(SEG_B_GPIO, 0);
            gpio_set_level(SEG_C_GPIO, 1);
            gpio_set_level(SEG_D_GPIO, 1);
            gpio_set_level(SEG_E_GPIO, 1);
            gpio_set_level(SEG_F_GPIO, 1);
            gpio_set_level(SEG_G_GPIO, 1);
            break;
        case 7: // Display '7'
            gpio_set_level(SEG_A_GPIO, 1);
            gpio_set_level(SEG_B_GPIO, 1);
            gpio_set_level(SEG_C_GPIO, 1);
            gpio_set_level(SEG_D_GPIO, 0);
            gpio_set_level(SEG_E_GPIO, 0);
            gpio_set_level(SEG_F_GPIO, 0);
            gpio_set_level(SEG_G_GPIO, 0);
            break;
        case 8: // Display '8'
            gpio_set_level(SEG_A_GPIO, 1);
            gpio_set_level(SEG_B_GPIO, 1);
            gpio_set_level(SEG_C_GPIO, 1);
            gpio_set_level(SEG_D_GPIO, 1);
            gpio_set_level(SEG_E_GPIO, 1);
            gpio_set_level(SEG_F_GPIO, 1);
            gpio_set_level(SEG_G_GPIO, 1);
            break;
        case 9: // Display '9'
            gpio_set_level(SEG_A_GPIO, 1);
            gpio_set_level(SEG_B_GPIO, 1);
            gpio_set_level(SEG_C_GPIO, 1);
            gpio_set_level(SEG_D_GPIO, 1);
            gpio_set_level(SEG_E_GPIO, 0);
            gpio_set_level(SEG_F_GPIO, 1);
            gpio_set_level(SEG_G_GPIO, 1);
            break;
        default:
            printf("Invalid setup_cursor value\n");
            break;

        
    }

}


void Fading_Task(void *params)
{
    while(1)
    {
        if(isLed_menu && menu_cursor ==0)
        {
            gpio_set_level(LED_BLUE2,1);
            gpio_set_level(LED_BLUE1,0);
            vTaskDelay(500/portTICK_PERIOD_MS);
            gpio_set_level(LED_BLUE1,1);

        }
        else if (isLed_menu && menu_cursor ==1)
        {
            gpio_set_level(LED_BLUE1,1);
            gpio_set_level(LED_BLUE2,0);
            vTaskDelay(500/portTICK_PERIOD_MS);
            gpio_set_level(LED_BLUE2,1);
        }

        else if(isLed_setup)
        {
            gpio_set_level(LED_BLUE1, 0);
        }
        else if(isLed_walk)
        {
            gpio_set_level(LED_BLUE2, 0);
        }
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
}

void Blink_Task(void *params)
{
    while(1)
    {
        if(isLed_setup)
        {
            // Display the current number on the seven-segment display
            display_number(setup_cursor);
            
            // Wait for a short period to create the blink effect
            vTaskDelay(400/ portTICK_PERIOD_MS);
    
            // Turn off all segments to create the blink effect
            turn_off_segments();
    
            // Wait for a short period to create the blink effect
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        else if(isLed_menu)
        {
                gpio_set_direction(LED_BLUE1, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_BLUE2, GPIO_MODE_OUTPUT);
    gpio_set_level(SEG_A_GPIO, 0);
    gpio_set_level(SEG_B_GPIO, 0);
    gpio_set_level(SEG_C_GPIO, 0);
    gpio_set_level(SEG_D_GPIO, 0);
    gpio_set_level(SEG_E_GPIO, 0);
    gpio_set_level(SEG_F_GPIO, 0);
    gpio_set_level(SEG_G_GPIO, 1);
        }
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
}


void VBUS_INTRTask(void *params)
{
    gpio_set_direction(VBUS_INTR, GPIO_MODE_INPUT);
    gpio_set_intr_type(VBUS_INTR, GPIO_INTR_POSEDGE);
    int BTN_NUMBER = 0;
    while (1)
    {
        if (xQueueReceive(VBUS_INTRQueue, &BTN_NUMBER, portMAX_DELAY))
        {
            
            if(vbus_flag)
            {
                printf("Charging....!\n");
                vbus_flag=false;
                isLed_menu = false;
                isLed_setup = false;
                isLed_walk = false;
                gpio_set_level(LED_BLUE1,1);
                gpio_set_level(LED_BLUE2,1);
                gpio_set_direction(RGB_GREEN,GPIO_MODE_OUTPUT);
                gpio_set_level(RGB_GREEN,0);
            gpio_set_level(SEG_A_GPIO, 1);
            gpio_set_level(SEG_B_GPIO, 1);
            gpio_set_level(SEG_C_GPIO, 0);
            gpio_set_level(SEG_D_GPIO, 0);
            gpio_set_level(SEG_E_GPIO, 1);
            gpio_set_level(SEG_F_GPIO, 1);
            gpio_set_level(SEG_G_GPIO, 1);

            }
            else if(!vbus_flag)
            {
                printf("pluged out....!\n");
                vbus_flag=true;
                led_menu();
            }

  
            xQueueReset(VBUS_INTRQueue);
        }
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
}


void BTN1Task(void *params)
{
    gpio_set_direction(BTN1, GPIO_MODE_INPUT);
    gpio_set_intr_type(BTN1, GPIO_INTR_NEGEDGE);
    int BTN_NUMBER = 0;

    while (1)
    {
        if (xQueueReceive(BTN1Queue, &BTN_NUMBER, portMAX_DELAY))
        {
            // Wait for button release
            while (gpio_get_level(BTN1) == 0)
            {
                btn1curr_time = esp_timer_get_time();
            }

            // Check if the button was pressed for a short duration
            if (btn1curr_time - btn1intr_time < 1000000) // Adjust the time threshold for short press detection as needed
            {
                ESP_LOGI("NO TAG", "Short Press Detected on BTN1");
                if(isLed_setup && setup_cursor<9)
                {
                    setup_cursor++;
                    led_setup();
                }
                else if(isLed_menu && menu_cursor==1)
                {
                    menu_cursor=0;
                    led_menu();
                }

            }

            xQueueReset(BTN1Queue);
        }
    }
}




// void BTN2Task(void *params)
// {
//     gpio_set_direction(BTN2, GPIO_MODE_INPUT);
//     gpio_set_intr_type(BTN2, GPIO_INTR_NEGEDGE);
//     int BTN_NUMBER = 0;
//     gpio_set_direction(LED_BLUE1, GPIO_MODE_OUTPUT);
//     gpio_set_direction(LED_BLUE2, GPIO_MODE_OUTPUT);

//     while (1)
//     {
//         if (xQueueReceive(BTN2Queue, &BTN_NUMBER, portMAX_DELAY))
//         {
//             long_press_detected = false; 
//             btn2intr_time = esp_timer_get_time();  // Record the interrupt time
            
//             // Wait for button release or long press detection
//             while (gpio_get_level(BTN2) == 0)
//             {
//                 btn2curr_time = esp_timer_get_time();

//                 // Check for long press duration
//                 if (btn2curr_time - btn2intr_time >= 1500000) // 1 second threshold for long press
//                 {
//                     ESP_LOGI("NO TAG", "Long Press Detected on BTN2");
//                     long_press_detected = true; // Set long press flag
//                     enter_deep_sleep(); 
//                     break; // Exit loop after long press is detected
//                 }
//             }

//             // Check if the button was released for a short press duration
//             if (!long_press_detected && (btn2curr_time - btn2intr_time < 1500000))
//             {
//                 ESP_LOGI("NO TAG", "Short Press Detected on BTN2");

//                 if (isLed_menu && menu_cursor == 0)
//                 {
//                     isLed_menu = false;
//                     isLed_setup = true;
                    
//                     led_setup();
//                 }
//                 else if (isLed_menu && menu_cursor == 1)
//                 {
//                     isLed_menu = false;
//                     isLed_walk = true;
//                     led_walk();
//                 }
//                 else if (isLed_setup)
//                 {
//                     isLed_menu = true;
//                     isLed_setup = false;
//                     led_menu();
//                 }
//                 else if (isLed_walk)
//                 {
//                     led_menu();
//                 }
//             }

//             xQueueReset(BTN2Queue); // Reset the queue after processing
//         }
//     }
// }

void BTN2Task(void *params)
{
    gpio_set_direction(BTN2, GPIO_MODE_INPUT);
    gpio_set_intr_type(BTN2, GPIO_INTR_NEGEDGE); // Trigger on falling edge (button press)
    int BTN_NUMBER = 0;

    while (1)
    {
        if (xQueueReceive(BTN2Queue, &BTN_NUMBER, portMAX_DELAY))
        {
            btn2intr_time = esp_timer_get_time(); // Capture the time when the button is pressed

            // Wait for button release
            while (gpio_get_level(BTN2) == 0)
            {
                vTaskDelay(10 / portTICK_PERIOD_MS); // Small delay to avoid CPU hogging
            }

            // Calculate press duration after button is released
            btn2curr_time = esp_timer_get_time();
            uint64_t press_duration = btn2curr_time - btn2intr_time;

            if (press_duration >= 1000000) // Check for long press duration
            {
                ESP_LOGI("NO TAG", "Long Press Detected");
                    long_press_detected = true; // Set long press flag
                    enter_deep_sleep(); 
                    // break; // Exit loop after long press is detected
            }
            else
            {
                ESP_LOGI("NO TAG", "Short Press Detected");
                if (isLed_menu && menu_cursor == 0)
                {
                    isLed_menu = false;
                    isLed_setup = true;
                    
                    led_setup();
                }
                else if (isLed_menu && menu_cursor == 1)
                {
                    isLed_menu = false;
                    isLed_walk = true;
                    led_walk();
                }
                else if (isLed_setup)
                {
                    isLed_menu = true;
                    isLed_setup = false;
                    led_menu();
                }
                else if (isLed_walk)
                {
                    led_menu();
                }
            }

            xQueueReset(BTN2Queue);
        }
    }
}


void BTN3Task(void *params)
{
    gpio_set_direction(BTN3, GPIO_MODE_INPUT);
    gpio_set_intr_type(BTN3, GPIO_INTR_NEGEDGE);
    int BTN_NUMBER = 0;

    while (1)
    {
        if (xQueueReceive(BTN3Queue, &BTN_NUMBER, portMAX_DELAY))
        {
            // Wait for button release
            while (gpio_get_level(BTN3) == 0)
            {
                btn3curr_time = esp_timer_get_time();
            }

            // Check if the button was pressed for a short duration
            if (btn3curr_time - btn3intr_time < 1000000) // Adjust the time threshold for short press detection as needed
            {
                ESP_LOGI("NO TAG", "Short Press Detected on BTN3");

                if(isLed_setup && setup_cursor>0)
                {
                    setup_cursor--;
                    led_setup();
                }

                else if(isLed_menu && menu_cursor==0)
                {
                    menu_cursor=1;
                    led_menu();
                }

            }

            xQueueReset(BTN3Queue);
        }
    }
}

static void IRAM_ATTR BTN1_interrupt_handler(void *args)
{
    int pinNumber = (int)args;

    if (esp_timer_get_time() - btn1pre_time > 400000)
    {
        xQueueSendFromISR(BTN1Queue, &pinNumber, NULL);
        btn1intr_time = esp_timer_get_time();
    }

    btn1pre_time = esp_timer_get_time();
}

// static void IRAM_ATTR BTN2_interrupt_handler(void *args)
// {
//     int pinNumber = (int)args;

//     if (esp_timer_get_time() - btn2pre_time > 400000)
//     {
//         xQueueSendFromISR(BTN2Queue, &pinNumber, NULL);
//         btn2intr_time = esp_timer_get_time();
//     }

//     btn2pre_time = esp_timer_get_time();
// }
static void IRAM_ATTR BTN2_interrupt_handler(void *args)
{
    int pinNumber = (int)args;

    if (esp_timer_get_time() - btn2curr_time > 400000)
    {
        xQueueSendFromISR(BTN2Queue, &pinNumber, NULL);
    }

    btn2curr_time = esp_timer_get_time();
}


static void IRAM_ATTR BTN3_interrupt_handler(void *args)
{
    int pinNumber = (int)args;

    if (esp_timer_get_time() - btn3pre_time > 400000)
    {
        xQueueSendFromISR(BTN3Queue, &pinNumber, NULL);
        btn3intr_time = esp_timer_get_time();
    }

    btn3pre_time = esp_timer_get_time();
}

static void IRAM_ATTR VBUS_INTR_interrupt_handler(void *args)
{
    int pinNumber = (int)args;
    if(esp_timer_get_time() - pre_time_vbus_intr > 400000){
    xQueueSendFromISR(VBUS_INTRQueue, &pinNumber, NULL);
    }
    pre_time_vbus_intr = esp_timer_get_time();
}



void app_main(void)
{
        BTN1Queue = xQueueCreate(10, sizeof(int));
    BTN2Queue = xQueueCreate(10, sizeof(int));
    BTN3Queue = xQueueCreate(10, sizeof(int));
     VBUS_INTRQueue = xQueueCreate(10, sizeof(int));

        gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN1, BTN1_interrupt_handler, (void *)BTN1);
    gpio_isr_handler_add(BTN2, BTN2_interrupt_handler, (void *)BTN2);
    gpio_isr_handler_add(BTN3, BTN3_interrupt_handler, (void *)BTN3);
    gpio_isr_handler_add(VBUS_INTR, VBUS_INTR_interrupt_handler, (void *)VBUS_INTR);

        xTaskCreate(BTN1Task, "BTN1_Task", 2048, NULL, 1, NULL);
    xTaskCreate(BTN2Task, "BTN2_Task", 2048, NULL, 1, NULL);
    xTaskCreate(BTN3Task, "BTN3_Task", 2048, NULL, 1, NULL);
    xTaskCreate(Blink_Task, "Blink_Task", 2048, NULL, 1, NULL);
    xTaskCreate(Fading_Task, "Fading_Task", 2048, NULL, 1, NULL);
    xTaskCreate(VBUS_INTRTask, "VBUS_INTRTask", 8000, NULL, 1, NULL);

    led_menu();

}
