//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


/// @file
/// @brief Contains classes and functions for manipulating dates and times


#ifndef TCRI_ZERO_TIME_H
#define TCRI_ZERO_TIME_H


#include <stdint.h>


namespace zero {

    /// @brief Simple class to specify the passage of time
    class Duration {
    public:

        /// @brief Creates a new Duration object
        /// @param v The number of milliseconds to be represented by the object
        explicit constexpr Duration( const uint32_t v )
        :
            _v{ v }
        {
            // empty
        }

        /// @brief Gets the number of millseconds represented by the object
        explicit operator uint32_t() const
        {
            return (uint32_t) _v;
        }

    private:
        const uint32_t _v;
    };


    /// @brief Creates a literal constant Duration of a given number of milliseconds
    /// @param v The number of milliseconds to be represented by the new Duration object.
    inline constexpr Duration operator""_ms( const unsigned long long int v )
    {
        return Duration{ (uint32_t) v };
    }


    /// @brief Creates a literal constant Duration of a given number of seconds
    /// @param v The number of seconds to be represented by the new Duration object.
    inline constexpr Duration operator""_secs( const unsigned long long int v )
    {
        return Duration{ (uint32_t) v * 1000 };
    }


    /// @brief Creates a literal constant Duration of a given number of minutes
    /// @param v The number of minutes to be represented by the new Duration object.
    inline constexpr Duration operator""_mins( const unsigned long long int v )
    {
        return Duration{ (uint32_t) v * 1000 * 60 };
    }


    /// @brief Creates a literal constant Duration of a given number of hours
    /// @param v The number of hours to be represented by the new Duration object.
    inline constexpr Duration operator""_hrs( const unsigned long long int v )
    {
        return Duration{ (uint32_t) v * 1000 * 60 * 60 };
    }


    /// @brief Creates a literal constant Duration of a given number of days
    /// @param v The number of days to be represented by the new Duration object.
    inline constexpr Duration operator""_days( const unsigned long long int v )
    {
        return Duration{ (uint32_t) v * 1000 * 60 * 60 * 24 };
    }


    /// @brief Creates a literal constant Duration of a given number of weeks
    /// @param v The number of weeks to be represented by the new Duration object.
    inline constexpr Duration operator""_wks( const unsigned long long int v )
    {
        return Duration{ (uint32_t) v * 1000 * 60 * 60 * 24 * 7 };
    }

}


#endif