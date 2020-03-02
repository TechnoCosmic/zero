//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_TIME_H
#define TCRI_ZERO_TIME_H


#include <stdint.h>


namespace zero {

    class Duration {
    public:

        explicit constexpr Duration( const uint32_t v )
        :
            _v{ v }
        {
            // empty
        }

        explicit operator uint32_t() const
        {
            return (uint32_t) _v;
        }

    private:
        const uint32_t _v;
    };


    inline constexpr Duration operator""_ms( const unsigned long long int v )
    {
        return Duration{ (uint32_t) v };
    }


    inline constexpr Duration operator""_secs( const unsigned long long int v )
    {
        return Duration{ (uint32_t) v * 1000 };
    }


    inline constexpr Duration operator""_mins( const unsigned long long int v )
    {
        return Duration{ (uint32_t) v * 1000 * 60 };
    }


    inline constexpr Duration operator""_hrs( const unsigned long long int v )
    {
        return Duration{ (uint32_t) v * 1000 * 60 * 60 };
    }


    inline constexpr Duration operator""_days( const unsigned long long int v )
    {
        return Duration{ (uint32_t) v * 1000 * 60 * 60 * 24 };
    }


    inline constexpr Duration operator""_wks( const unsigned long long int v )
    {
        return Duration{ (uint32_t) v * 1000 * 60 * 60 * 24 * 7 };
    }

}


#endif