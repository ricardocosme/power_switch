#pragma once

#include <avr/interrupt.hpp>
#include <avr/io.hpp>
#include <avr/sleep.hpp>

namespace power {

inline volatile bool request_turnon{false};
inline volatile bool fst_sleep{true};

template<typename SwitchPin, typename PowerPin>
[[gnu::always_inline]]
inline void turnon(SwitchPin sw, PowerPin power) {
    using namespace avr::io;
    clear(pcint_to(sw));
    if(fst_sleep) {
        request_turnon = true;
        power.high();
    } else {
        using namespace avr::wdt;
        avr::wdt::on(timeout::at_16ms, mode::reset, atomic_precondition::yes);
        while(true);
    }
}

template<typename SwitchPin,
         typename PowerPin,
         typename BtnPressed,
         typename BtnReleased>
class sw {
    bool _request_turnoff{false};
    SwitchPin _sw;
    PowerPin _power;
    BtnPressed _btn_pressed;
    BtnReleased _btn_released;
    
    void startup() {
        using namespace avr;
        using namespace avr::io;
        _power.out();
        
        set(pcie);
        set(pcint_to(_sw));
    
        interrupt::on();
        sleep::power_down::sleep();
        fst_sleep = false;
    }
    
    void wait_turnon() {
        while(true) {
            if(_btn_released(_sw)) {
                request_turnon = false;
                return;
            }
        }
    }
    
    struct do_nothing{ void operator()() const {} };
public:
    sw() = default;
    
    explicit sw(SwitchPin sw,
                PowerPin power,
                BtnPressed btn_pressed,
                BtnReleased btn_released)
        : _sw(sw)
        , _power(power)
        , _btn_pressed(btn_pressed)
        , _btn_released(btn_released)
    {
        using namespace avr::io;
        if(!wdrf.state()) {
            clear(porf);
            _sw.in(avr::io::pullup);
            startup();
            wait_turnon();
        } else {
            avr::wdt::off();
            fst_sleep = false;
            _sw.in(avr::io::pullup);
            _power.out();
            _power.high();
            while(true)
                if(_btn_released(_sw)) break;
            set(pcie);
            avr::interrupt::on();
        }
    }
    
    template<typename OnTurnOff = do_nothing>
    [[gnu::always_inline]]
    inline void poll(OnTurnOff turnoff_cbk = do_nothing{}) {
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
                    off(turnoff_cbk);
                }
            }
        }
    }

    template<typename OnTurnOff = do_nothing>
    void off(OnTurnOff turnoff_cbk = do_nothing{}) {
        using namespace avr;
        using namespace avr::io;
        {
            interrupt::atomic s{interrupt::on_at_the_end};
            set(pcint_to(_sw));
            _power.low();
            turnoff_cbk();
            sleep::power_down::on();
        }
        sleep::sleep();
        sleep::power_down::off();
    }
};

}
