#include "gpio.h"
#include "rcc.h"

#define EXTI_BASE 0x40010400
#define EXTI ((EXTI_t *)EXTI_BASE)

#define EXTI15_10_IRQn 40
#define NVIC_ISER1 ((uint32_t *)(0xE000E104)) // NVIC Interrupt Set-Enable Register

#define EXTI0_IRQn 6 //Se define la posicion del vector donde esta EXTI0
#define EXTI1_IRQn 7 //Se define la posicion del vector donde esta EXTI1
#define NVIC_ISER0 ((uint32_t*)(0xE000E100)) //Se hace el registro de activacion y configuracion de interrupciones NVIC para EXIT 0 y EXIT 1

#define SYSCFG_BASE 0x40010000
#define SYSCFG ((SYSCFG_t *)SYSCFG_BASE)


#define GPIOA ((GPIO_t *)0x48000000) // Base address of GPIOA
#define GPIOC ((GPIO_t *)0x48000800) // Base address of GPIOC

#define LED_PIN 5 // Pin 5 of GPIOA (Heartbeat)(Led que titila)
#define LED1_PIN 0 // Pin 0 of GPIOA
#define LED2_PIN 1 // El 1 es el pin del microcontrolador que se conecta al Morpho

#define BUTTON_PIN 13 // Pin 13 of GPIOC
#define BUTTON1_PIN 0 
#define BUTTON2_PIN 1 

#define BUTTON_IS_PRESSED()    (!(GPIOC->IDR & (1 << BUTTON_PIN))) //Comprueba si el boton esta precionado
#define BUTTON_IS_RELEASED()   (GPIOC->IDR & (1 << BUTTON_PIN))
#define TOGGLE_LED()           (GPIOA->ODR ^= (1 << LED_PIN))

#define BUTTON1_IS_PRESSED()    (!(GPIOC->IDR & (1 << BUTTON1_PIN))) //Se configura el boton
#define BUTTON1_IS_RELEASED()   (GPIOC->IDR & (1 << BUTTON1_PIN))
#define TOGGLE_LED1()           (GPIOA->ODR ^= (1 << LED1_PIN))
 
#define BUTTON2_IS_PRESSED()    (!(GPIOC->IDR & (1 << BUTTON2_PIN))) 
#define BUTTON2_IS_RELEASED()   (GPIOC->IDR & (1 << BUTTON2_PIN))
#define TOGGLE_LED2()           (GPIOA->ODR ^= (1 << LED2_PIN))

volatile uint8_t button_pressed = 0; // Flag to indicate button press

void configure_gpio_for_usart() {
    // Enable GPIOA clock
    *RCC_AHB2ENR |= (1 << 0);

    // Configure PA2 (TX) as alternate function
    GPIOA->MODER &= ~(3U << (2 * 2)); // Clear mode bits for PA2
    GPIOA->MODER |= (2U << (2 * 2));  // Set alternate function mode for PA2

    // Configure PA3 (RX) as alternate function
    GPIOA->MODER &= ~(3U << (3 * 2)); // Clear mode bits for PA3
    GPIOA->MODER |= (2U << (3 * 2));  // Set alternate function mode for PA3

    // Set alternate function to AF7 for PA2 and PA3
    GPIOA->AFR[0] &= ~(0xF << (4 * 2)); // Clear AFR bits for PA2
    GPIOA->AFR[0] |= (7U << (4 * 2));   // Set AFR to AF7 for PA2
    GPIOA->AFR[0] &= ~(0xF << (4 * 3)); // Clear AFR bits for PA3
    GPIOA->AFR[0] |= (7U << (4 * 3));   // Set AFR to AF7 for PA3

    // Configure PA2 and PA3 as very high speed
    GPIOA->OSPEEDR |= (3U << (2 * 2)); // Very high speed for PA2
    GPIOA->OSPEEDR |= (3U << (3 * 2)); // Very high speed for PA3

    // Configure PA2 and PA3 as no pull-up, no pull-down
    GPIOA->PUPDR &= ~(3U << (2 * 2)); // No pull-up, no pull-down for PA2
    GPIOA->PUPDR &= ~(3U << (3 * 2)); // No pull-up, no pull-down for PA3
}

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
    SYSCFG->EXTICR[3] &= ~(0xF << 4); // Clear bits for EXTI3
    SYSCFG->EXTICR[3] |= (0x2 << 4);  // Map EXTI13 to Port C 

    // Configure SYSCFG EXTICR para asignar EXTI0 a PC0 Y EXTI1 a PC1

    SYSCFG->EXTICR[0] &= ~(0xF << 0); // Clear bits for EXTI0
    SYSCFG->EXTICR[0] |= (0x2 << 0);  // Map EXTI0 to Port C
     SYSCFG->EXTICR[0] &= ~(0xF << 4); // Clear bits for EXTI1
    SYSCFG->EXTICR[0] |= (0x2 << 4);  // Map EXTI1 to Port C

    // Configure EXTI13 for falling edge trigger
    EXTI->FTSR1 |= (1 << BUTTON_PIN);  // Enable falling trigger
    EXTI->RTSR1 &= ~(1 << BUTTON_PIN); // Disable rising trigger

    // Configura EXTI0 para que se ejecute en el flanco
    EXTI->FTSR1 |= (1 << BUTTON1_PIN);  // Hablita flanco de bajada
    EXTI->RTSR1 &= ~(1 << BUTTON1_PIN); // Desabilita flanco de subida

    // Configura EXTI0 para que se ejecute en el flanco
    EXTI->FTSR1 |= (1 << BUTTON2_PIN);  // Hablita flanco de bajada
    EXTI->RTSR1 &= ~(1 << BUTTON2_PIN); // Desabilita flanco de subida

    // Unmask EXTI13
    EXTI->IMR1 |= (1 << BUTTON_PIN);

    // Interrupcion habilitada para EXTI0
    EXTI->IMR1 |= (1 << BUTTON1_PIN);

    // Interrupcion habilitada par EXTI1
    EXTI->IMR1 |= (1 << BUTTON2_PIN);

    init_gpio_pin(GPIOA, LED_PIN, 0x1); // Set LED pin as output
    init_gpio_pin(GPIOA, LED1_PIN, 0x1); //Se establece LED1 como salida
    init_gpio_pin(GPIOA, LED2_PIN, 0x1); //Se establece LED2 como salida

    init_gpio_pin(GPIOC, BUTTON_PIN, 0x0); // Set BUTTON pin as input
    init_gpio_pin(GPIOC, BUTTON1_PIN, 0x0); // Se establece botón 1 como entrada
    init_gpio_pin(GPIOC, BUTTON2_PIN, 0x0); // Se establece botón 2 como entrada

    // Enable EXTI15_10 interrupt
    *NVIC_ISER1 |= (1 << (EXTI15_10_IRQn - 32));

    // habilitar interrupcion EXT0
    *NVIC_ISER0 |= (1 << (EXTI0_IRQn));

    // habilitar interrupcion EXT1
    *NVIC_ISER1 |= (1 << (EXTI1_IRQn));
   
    configure_gpio_for_usart();
}

uint8_t gpio_button_is_pressed(void)
{
    return BUTTON_IS_PRESSED();
}

uint8_t gpio_button1_is_pressed(void)
{
    return BUTTON1_IS_PRESSED();
}

uint8_t gpio_button2_is_pressed(void)
{
    return BUTTON2_IS_PRESSED();
}


void gpio_toggle_led(void)
{
    TOGGLE_LED();
}

void gpio_toggle_led1(void)
{
    TOGGLE_LED1();
}

void gpio_toggle_led2(void)
{
    TOGGLE_LED2();
}

void EXTI15_10_IRQHandler(void)
{
    if (EXTI->PR1 & (1 << BUTTON_PIN)) {
        EXTI->PR1 = (1 << BUTTON_PIN); // Clear pending bit
        button_pressed = 1; // Set button pressed flag
    }
}

void EXTI0_IRQHandler(void)
{
    if (EXTI->PR1 & (1 << BUTTON1_PIN)) {
        EXTI->PR1 = (1 << BUTTON1_PIN); // Clear pending bit
        button_pressed = 1; // Set button pressed flag
    }
}

void EXTI1_IRQHandler(void)
{
    if (EXTI->PR1 & (1 << BUTTON2_PIN)) {
        EXTI->PR1 = (1 << BUTTON2_PIN); // Clear pending bit
        button_pressed = 1; // Set button pressed flag
    }
}