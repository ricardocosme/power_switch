#pragma once

#include <avr/interrupt.hpp>
#include <avr/io.hpp>
#include <avr/sleep.hpp>

namespace on_off {

inline volatile bool request_turnon{false};

template<typename SwitchPin, typename PowerPin>
[[gnu::always_inline]]
inline void turnon(SwitchPin sw, PowerPin power) {
    clear(pcint_to(sw));
    request_turnon = true;
    power.high();
}

template<typename SwitchPin,
         typename PowerPin,
         typename BtnPressed,
         typename BtnReleased>
class on_off {
    bool _request_turnoff{false};
    SwitchPin _sw;
    PowerPin _power;
    BtnPressed _btn_pressed;
    BtnReleased _btn_released;
    
    [[gnu::always_inline]]
    inline void startup() {
        using namespace avr;
        using namespace avr::io;
        _power.out();
        
        set(pcie);
        set(pcint_to(_sw));
    
        interrupt::on();
        sleep::power_down::sleep();
    }
public:
    on_off() = default;
    
    explicit on_off(SwitchPin sw,
                    PowerPin power,
                    BtnPressed btn_pressed,
                    BtnReleased btn_released)
        : _sw(sw)
        , _power(power)
        , _btn_pressed(btn_pressed)
        , _btn_released(btn_released)
    {
        _sw.in(avr::io::pullup);
        startup();
    }
    
    [[gnu::always_inline]]
    inline void poll() {
        using namespace avr;
        using namespace avr::io;

        if(request_turnon){
            if(_btn_released(_sw)) request_turnon = false;
        } else {
            if(!_request_turnoff) {
                if(_btn_pressed(_sw)) _request_turnoff = true;
            } else {
                if(_btn_released(_sw)) {
                    _request_turnoff = false;
                    {
                        interrupt::atomic s{interrupt::on_at_the_end};
                        set(pcint_to(_sw));
                        _power.low();
                        sleep::power_down::on();
                    }
                    sleep::sleep();
                    sleep::power_down::off();
                }
            }
        }
    }
};

}
