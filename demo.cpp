#include <on_off.hpp>
#define F_CPU 1e6
#include <util/delay.h>

using namespace avr::io;

/** This demo illustrates how to use only one momentary switch and two pins 
    to implement a power switch.

    The load can be represented by a LED connected to the pin PB0 that
    can be turned on or off by a push button connected to the pin
    PB2. When the MCU is started it goes to sleep using the power-down
    sleep mode. If the switch is pressed the MCU wakes up and turns on
    the LED. If the switch is pressed again the MCU turns off the LED
    and goes to sleep.
 */

static auto& load{pb0};
static auto& sw{pb2};

/** ISR to turn on the system when the MCU is sleeping. */
AVRINT_PCINT0(){ on_off::turnon(sw, load); }

int main() {
    /** A naive implementation to debounces a pressed push button that
        uses a pull-up resistor. */
    auto btn_pressed = [](auto pin){
        if(pin.is_low()) {
            _delay_ms(40);
            return pin.is_low();
        } 
        return false;
    };
    
    /** A naive implementation to debounces a released push button. */
    auto btn_released = [](auto pin){
        if(pin.is_high()) {
            _delay_ms(40);
            return pin.is_high();
        } 
        return false;
    };
    
    /** The system is halted at the startup and it turns on when the
        switch is pressed. */
    on_off::on_off power(sw, load, btn_pressed, btn_released);

    while(true) {
        /** Polling to check if the system should be turned off. */
        power.poll();
    }
}
