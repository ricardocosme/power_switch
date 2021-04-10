#include <avr/io.hpp>
#include <avr/sleep.hpp>
#define F_CPU 1e6
#include <util/delay.h>

using namespace avr;
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

/** Flag to request a turn on of the system. */
volatile bool turnon{false};

/** Pin that delivers power to the system. */
static auto& load{pb0};

/** ISR to turn on the system when the MCU is sleeping. */
AVRINT_PCINT0(){
    clear(pcint2);
    turnon = true;
    load.high();
}

/** A naive implementation to debounces a pressed push button that
    uses a pull-up resistor. */
template<typename Pin>
bool sw_pressed(Pin pin) {
    if(pin.is_low()) {
        _delay_ms(40);
        return pin.is_low();
    } 
    return false;
}

/** A naive implementation to debounces a released push button. */
template<typename Pin>
bool sw_released(Pin pin) {
    if(pin.is_high()) {
        _delay_ms(40);
        return pin.is_high();
    } 
    return false;
}

int main() {
    /** Momentary power switch used to turn on or off the system. */
    Pb2 on_off{pullup};

    /** The system is halted at the startup and it turns on when the
        switch is pressed. */
    load.out();
    set(pcie);
    set(pcint2);
    interrupt::on();
    sleep::power_down::sleep();
    
    /** Flag to request a turn off of the system. */
    bool turnoff{false};
    while(true) {
        /** Polling to check if the system should be turned off. */
        if(turnon){
            if(sw_released(on_off)) turnon = false;
        } else {
            if(!turnoff) {
                if(sw_pressed(on_off)) turnoff = true;
            } else {
                if(sw_released(on_off)) {
                    turnoff = false;
                    {
                        interrupt::atomic s{interrupt::on_at_the_end};
                        set(pcint2);
                        load.low();
                        sleep::power_down::on();
                    }
                    sleep::sleep();
                    sleep::power_down::off();
                }
            }
        }
    }
}
