#include <stdint.h>

#include "systick.h"

typedef struct {
    volatile uint32_t MEMRMP;
    volatile uint32_t CFGR1;
    volatile uint32_t EXTICR[4];
    volatile uint32_t CFGR2;
} SYSCFG_t;

typedef struct {
    volatile uint32_t IMR1;
    volatile uint32_t EMR1;
    volatile uint32_t RTSR1;
    volatile uint32_t FTSR1;
    volatile uint32_t SWIER1;
    volatile uint32_t PR1;
    volatile uint32_t IMR2;
    volatile uint32_t EMR2;
    volatile uint32_t RTSR2;
    volatile uint32_t FTSR2;
    volatile uint32_t SWIER2;
    volatile uint32_t PR2;
} EXTI_t;


typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
    volatile uint32_t BRR;

} GPIO_t;

#define EXTI_BASE 0x40010400
#define EXTI ((EXTI_t *)EXTI_BASE)

#define EXTI15_10_IRQn 40
#define NVIC_ISER1 ((uint32_t *)(0xE000E104)) // NVIC Interrupt Set-Enable Register


#define RCC_APB2ENR ((uint32_t *)(RCC_BASE + 0x60)) // APB2 peripheral clock enable register

#define SYSCFG_BASE 0x40010000
#define SYSCFG ((SYSCFG_t *)SYSCFG_BASE)

#define RCC_BASE 0x40021000
#define RCC_AHB2ENR ((uint32_t *)(RCC_BASE + 0x4C))

#define GPIOA ((GPIO_t *)0x48000000) // Base address of GPIOA
#define GPIOC ((GPIO_t *)0x48000800) // Base address of GPIOC

#define LED_PIN 5 // Pin 5 of GPIOA
#define BUTTON_PIN 13 // Pin 13 of GPIOC

#define BUTTON_IS_PRESSED()    (!(GPIOC->IDR & (1 << BUTTON_PIN)))
#define BUTTON_IS_RELEASED()   (GPIOC->IDR & (1 << BUTTON_PIN))
#define TOGGLE_LED()           (GPIOA->ODR ^= (1 << LED_PIN))

volatile uint8_t button_pressed = 0; // Flag to indicate button press



void init_gpio_pin(GPIO_t *GPIOx, uint8_t pin, uint8_t mode)
{
    GPIOx->MODER &= ~(0x3 << (pin * 2)); // Clear MODER bits for this pin
    GPIOx->MODER |= (mode << (pin * 2)); // Set MODER bits for this pin
}

void configure_gpio(void)
{
    *RCC_AHB2ENR |= (1 << 0) | (1 << 2); // Enable clock for GPIOA and GPIOC

    // Enable clock for SYSCFG
    *RCC_APB2ENR |= (1 << 0); // RCC_APB2ENR_SYSCFGEN

    // Configure SYSCFG EXTICR to map EXTI13 to PC13
    SYSCFG->EXTICR[3] &= ~(0xF << 4); // Clear bits for EXTI13
    SYSCFG->EXTICR[3] |= (0x2 << 4);  // Map EXTI13 to Port C

    // Configure EXTI13 for falling edge trigger
    EXTI->FTSR1 |= (1 << BUTTON_PIN);  // Enable falling trigger
    EXTI->RTSR1 &= ~(1 << BUTTON_PIN); // Disable rising trigger

    // Unmask EXTI13
    EXTI->IMR1 |= (1 << BUTTON_PIN);

    init_gpio_pin(GPIOA, LED_PIN, 0x1); // Set LED pin as output
    init_gpio_pin(GPIOC, BUTTON_PIN, 0x0); // Set BUTTON pin as input

    // Enable EXTI15_10 interrupt
    *NVIC_ISER1 |= (1 << (EXTI15_10_IRQn - 32));
}

int main(void)
{
    configure_systick_and_start();
    configure_gpio();

    uint8_t state = 0; // state of the FSM

    while (1) {
        switch (state) {
        case 0: // idle
            if (button_pressed != 0) { // If button is pressed
                state = 1;
            } else if (systick_GetTick() >= 500) { // Blink LED every 500 ms
                state = 2;
            }
            break;
        case 1: // button pressed
            if (BUTTON_IS_RELEASED()) { // If button is released
                button_pressed = 0; // Clear button pressed flag
                systick_reset(); // Reset counter
                state = 0;
            }
            break;
        case 2: // led toggle
            TOGGLE_LED(); // Toggle LED
            systick_reset(); // Reset counter
            state = 0;
            break;
        default:
            break;
        }
        
    }
}



void EXTI15_10_IRQHandler(void)
{
    if (EXTI->PR1 & (1 << BUTTON_PIN)) {
        EXTI->PR1 = (1 << BUTTON_PIN); // Clear pending bit
        button_pressed = 1; // Set button pressed flag
    }
}
