#include <stm32f0xx.h>
#include <stddef.h>
#include "Modbus/modbus_slave.h"
#define slave_id 26
#define VAR_N_bytes 18

union _variables {
    uint8_t var[VAR_N_bytes];
    #pragma pack(push, 1)
    struct {
	uint16_t reserved;
        uint16_t out[6];
        uint16_t in[2];
            };
    #pragma pack(pop)
};

union _variables var;

typedef struct {
    GPIO_TypeDef *gpio;
    uint16_t pin;
} _gpio;

_gpio port_out[6] = {
    {GPIOA, 10},
    {GPIOA, 9},
    {GPIOA, 7},
    {GPIOA, 6},
    {GPIOA, 5},
    {GPIOA, 4}
};

_gpio port_in[2] = {
    {GPIOA, 0},
    {GPIOB, 1}
    
};

void SetPin(_gpio *port, uint8_t value){
    port -> gpio -> BSRR = (value) ? (1 << port -> pin) : (1 << port -> pin << 16);
}

void InitPortOut(_gpio *port){
    port -> gpio -> MODER |= 1 << (port -> pin * 2);
    port -> gpio -> OSPEEDR |= 3 << (port -> pin * 2);
}

void InitPortIn(_gpio *port){
    port -> gpio -> MODER &= ~(3 << (port -> pin * 2));
}

uint16_t GetPin(_gpio *port){
    return (port -> gpio -> IDR & (1 << port -> pin)) ? 1 : 0; 
}

void MODBUS_Slave_Changed_Handler(uint16_t addr_mas, uint16_t length_byte){
    /*switch (addr_mas){
       case (offsetof(union _variables, set_time)): 
       case (offsetof(union _variables, set_value)):
            if (var.set_time == 0) break;
            if (var.set_value > 100) var.set_value = 100;
            var.step_value = (float)fabs(var.get_value - var.set_value) / var.set_time * 100;
            break;
        default:
            break;
    }*/
}

void SetPll48(){
    RCC -> CR |= RCC_CR_HSEON;
    while (!(RCC -> CR & RCC_CR_HSERDY));
    RCC -> CFGR |= RCC_CFGR_PLLMUL6 | RCC_CFGR_PLLSRC_HSE_PREDIV;
    RCC -> CR |= RCC_CR_PLLON;
    while (!(RCC -> CR & RCC_CR_PLLRDY));
    RCC -> CFGR |= RCC_CFGR_SW_PLL;
}


void main(){
    uint8_t i;

    SetPll48();
    SystemCoreClockUpdate();
    Modbus_Slave_Init(slave_id, var.var);// slave adr

    RCC -> AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;
    for (i = 0; i < 6; i++){
        InitPortOut(&port_out[i]);
    }
    for (i = 0; i < 2; i++){
        InitPortIn(&port_in[i]);
    }

    IWDG -> KR = 0x0000CCCC;
    IWDG -> KR = 0x00005555;
    IWDG -> PR = 3;
    IWDG -> RLR = 0xFFF;
    while (IWDG -> SR);
    IWDG -> KR = 0x0000AAAA;

    while(1){
        IWDG -> KR = 0x0000AAAA;
        for (i = 0; i < 6; i++){
            SetPin(&port_out[i], var.out[i]);
        }
        for (i = 0; i < 2; i++){
            var.in[i] = GetPin(&port_in[i]);
        }
    }
}