#include <type_traits>
#include <cmath>

template <typename T, typename Stream>
auto parse_integral(Stream& stream)
-> std::enable_if_t<std::is_integral<T>::value, T> {
    char c;
    T value = 0;
    int sign = 1;
    stream >> c;
    
    if (c == '+' || c == '-') {
        if constexpr (!std::is_unsigned<T>::value) {
            sign = (c=='-') ? -1 : 1;
        } else {
            throw std::runtime_error("unexpected sign while parsing unsigned value");
        }
    }
    else if ( !isdigit(c) ) { 
        throw std::runtime_error("expected digit");
    } else {
        value = (c-'0');
    }

    while( !stream.empty() ) {
        stream >> c;
        if ( isdigit(c) ) {
            value = value*10 + (c-'0');
        } else { 
            stream.back();
            break;
        }
    }
    return value * sign;
}

template <typename T, typename Stream>
auto parse_floating_point(Stream& stream)
-> std::enable_if_t<std::is_floating_point<T>::value, T> {
    char c;
    T value = 0;
    int sign = 1;
    bool fractional = false;
    size_t frac_digits = 0;

    stream >> c;
    if (c == '+' || c == '-') {
        sign = (c=='-') ? -1 : 1;
    }
    else if ( !isdigit(c) ) { 
        throw std::runtime_error("expected digit");
    } else {
        value = (c-'0');
    }

    while( !stream.empty() ) {
        stream >> c;
        if ( isdigit(c) ) {
            value = value*10 + (c-'0');
            if (fractional) ++frac_digits;
        }
        else if (c == '.' && !fractional) {
            fractional = true;
            frac_digits = 0;
        } else { 
            stream.back();
            break;
        }
    }
    return sign * value / std::pow(10, frac_digits);
}