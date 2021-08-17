#include <iostream>
#include <type_traits>
#include <array>
#include <cstring> //strlen
#include "common_parsers.hpp"

#define PARSE_SCHEME(T)     \
template <>                 \
template <typename Stream>  \
auto value< T >::parse(Stream& stream) const -> T


namespace Parsec {

class ParseScheme {};

template <typename T>
struct token_stream;

template <typename T>
struct stream_state;

template <typename T>
struct Match;

template <typename T>
struct value;

template<>
struct stream_state<const char *> {
    size_t pos;
};

template<>
struct token_stream<const char *> {
    token_stream(const char * s) 
    : str {s} 
    , len {std::strlen(s)}
    , pos {0}
    {}

    auto save_state() const -> stream_state<const char *> {
        return { pos };
    }

    void load_state(stream_state<const char *> state) {
        pos = state.pos;
    }

    void back(size_t n=1) {
        if (pos > 0) pos -= n;
    }

    bool empty() { return pos >= len; }

    friend token_stream&
    operator>> (token_stream& s, char& c) {
        c = (s.pos < s.len)? s.str[s.pos++] : throw "";
        return s;
    }

    const char * str;
    size_t len = 0;
    size_t pos = 0;
};

template <typename Scheme>
class Parser : ParseScheme {
public:
    Parser (Scheme s) : scheme{s} {}

    template <typename T>
    auto parse(T s) {
        auto stream = token_stream<const char *>(s); // make token_stream from s
        return scheme.parse(stream);
    }
private:
    Scheme scheme;
};

template <typename Scheme>
Parser<Scheme> make_parser (Scheme s) {
    return {s};
}

template <typename... T1, typename... T2>
auto tuple_add(std::tuple<T1...> t1, std::tuple<T2...> t2) 
-> std::tuple<T1..., T2...> {
    return std::tuple_cat(t1, t2);
}

template <typename... Ts, typename U>
auto tuple_add(std::tuple<Ts...> t, U val)  
-> std::tuple<Ts..., U> {
    return std::tuple_cat(t, std::make_tuple(val));
}

template <typename U, typename... Ts>
auto tuple_add(U val, std::tuple<Ts...> t)  
-> std::tuple<U, Ts...> {
    return std::tuple_cat(std::make_tuple(val), t);
}

template <typename U, typename V>
auto tuple_add(U u, V v)  
-> std::tuple<U, V> {
    return std::make_tuple(u,v);
}

// tuple concat of ret_type(s) for S1 and S2:

template <typename T1, typename T2, size_t Case>
struct _chaining;

template <typename T, bool Flag>
struct _unwrap {
    using type = T;
};

template <typename T>
struct _unwrap<T, true> {
    using type = typename T::ret_type;
};

template <typename T>
using unwrap = typename _unwrap<T, std::is_base_of_v<ParseScheme, T> >::type;


template <typename T1, typename T2>
struct _chaining<T1,T2,3> {
    using ret_type = decltype(tuple_add(std::declval<unwrap<T1>>(), std::declval<unwrap<T2>>()));
};

template <typename T1, typename T2>
struct _chaining<T1,T2,1> {
    using ret_type = unwrap<T1>;
};

template <typename T1, typename T2>
struct _chaining<T1,T2,2> {
    using ret_type = unwrap<T2>;
};

template <typename T1, typename T2>
struct _chaining<T1,T2,0 > {
    using ret_type = void;
};

template <typename T1, typename T2>
using chaining_type = typename _chaining<T1,T2, T1::returns_value + T2::returns_value*2>::ret_type;


template <typename Scheme1, typename Scheme2>
class Chain : ParseScheme {
public:
    enum {
        returns_value = Scheme1::returns_value 
                        ||
                        Scheme2::returns_value
    };

    using ret_type = chaining_type<Scheme1, Scheme2>;

    Chain (Scheme1 s1, Scheme2 s2) 
    : first {s1}
    , second {s2}
    {}

    template <typename Stream>
    auto parse(Stream& stream) const {//-> ret_type {
        auto v1 = first.parse(stream);
        auto v2 = second.parse(stream);

        if constexpr (!returns_value) {
            return;
        }
        if constexpr (Scheme1::returns_value) {
            if constexpr (Scheme2::returns_value) {
                return tuple_add(v1, v2);
            }
            else {
                return v1;
            }
        } else {
            return v2;
        }
    }

    template <typename Stream>
    using deduced_t = std::result_of<decltype(&Chain::parse<Stream&>)(Stream&)>;
private:
    Scheme1 first;
    Scheme2 second;
};


template <size_t Times, typename Scheme>
class Repeat : ParseScheme {
public:
    enum {returns_value = Scheme::returns_value};
    using ret_type = std::array<typename Scheme::ret_type, Times>;

    Repeat (Scheme s) 
    : scheme {s}
    {}

    template <typename Stream>
    auto parse(Stream& stream) const {
        std::array<typename Scheme::ret_type, Times> arr;
        for (auto & elem : arr) {
            elem = scheme.parse( stream );
        }
        return arr;
    }

private:
    Scheme scheme;
};

template <
    class Scheme1,
    class Scheme2, 
    typename = std::enable_if_t<(
        std::is_base_of_v<ParseScheme, Scheme1> 
        &&
        std::is_base_of_v<ParseScheme, Scheme2>
    )>
> 
auto operator<< (Scheme1 s1, Scheme2 s2) 
-> Chain<Scheme1, Scheme2> {
    return Chain<Scheme1,Scheme2> {s1, s2};
}

template <class Scheme, typename=std::enable_if_t<
std::is_base_of_v<ParseScheme, Scheme>
>>
auto operator<< (Scheme s, const char * str) 
-> Chain< Scheme, Match<std::string> > {
    return {s, std::string(str)};
}

template <class Scheme>
auto operator<< (const char * str, Scheme s) 
-> Chain< Match<std::string>, Scheme > {
    return {std::string(str), s};
}

template <typename T>
struct Match : ParseScheme {
    enum {returns_value = false};
    using ret_type = void;
    
    Match (T value)
    : val{value}
    {}

    template <typename Stream>
    bool parse(Stream& stream) const {
        std::cout << '"'<<val<<"\"";
        auto parsed = value<T>().parse(stream);
        if (parsed != val) {
            throw std::runtime_error("parse_error");
        }
        return true;
    }

    T val;
};

template<>
template <typename Stream>
auto Match<std::string>::parse(Stream & stream) const
-> bool {
    char c;
    for (auto match_c : val) {
        if ( stream.empty() ) {
            throw std::runtime_error("match error: stream empty");
        } else {
            stream >> c;
            if (c != match_c) {
                throw std::runtime_error("match error");
            }
        }
    }
    return true;
}

template <typename T>
Match<T> match(T val) {return Match<T>(val);}

template <size_t N, typename Scheme>
Repeat<N,Scheme> repeat(Scheme s) {
    return Repeat<N,Scheme>(s);
}

template <typename T>
struct value : ParseScheme {
    enum {returns_value = true};
    using ret_type = T;

    template <typename Stream>
    T parse(Stream& stream) const;

    template <typename Stream>
    void encode(Stream& stream, T value) const;
};

struct delimited_string : ParseScheme {
    enum {returns_value = true};
    using ret_type = std::string;

    delimited_string(const char * del_seq)
    : delimiter { del_seq } {}

    template <typename Stream>
    std::string parse(Stream& stream) const {
        char c;
        std::string str;
        size_t del_idx = 0;
        auto state = stream.save_state();

        while( !stream.empty() ) {
            stream >> c;
            if (delimiter[del_idx] == '\0') {
                // delimiter fully matched, 
                // load stream state
                stream.load_state(state);
                return str;
            }
            else if (c == delimiter[del_idx]) {
                if ( del_idx == 0 ) {
                    state = stream.save_state();
                }
                del_idx += 1;
                
            } else {
                del_idx = 0;
                str += c;
            }
        }
        return str;
    }

    template <typename Stream>
    void encode(Stream& stream, std::string value) const {
        stream << value << delimiter;
    }

    const char * delimiter;
};

template <>
struct value<std::string> : ParseScheme {
    enum {returns_value = true};
    using ret_type = std::string;

    template <typename Stream>
    std::string parse(Stream& stream) const{
        // reads to the end of the stream
        char c;
        std::string str;
        while( !stream.empty() ) {
            stream >> c;
            str += c;
        }
        return str;
    }

    auto withSeparator(const char * sep) {
        return delimited_string(sep);
    }

    auto then(const char * sep) {
       return delimited_string(sep);
    }
};


// template <>
// template <typename Stream> 
// auto value<int>::parse(Stream& stream) const 
// -> int 
// { return parse_integral<int>( stream ); }

template <>
template <typename Stream> 
auto value<unsigned int>::parse(Stream& stream) const 
-> unsigned int 
{ return parse_integral<unsigned int>( stream ); }

PARSE_SCHEME(int) { return parse_integral<int>( stream ); }
PARSE_SCHEME(short) { return parse_integral<short>( stream ); }
PARSE_SCHEME(unsigned short) { return parse_integral<unsigned short>( stream ); }
PARSE_SCHEME(long) { return parse_integral<long>( stream ); }
PARSE_SCHEME(unsigned long) { return parse_integral<unsigned long>( stream ); }

template <>
template <typename Stream> 
auto value<float>::parse(Stream& stream) const
-> float
{ return parse_floating_point<float>( stream ); }

template <>
template <typename Stream> 
auto value<double>::parse(Stream& stream) const
-> double
{ return parse_floating_point<double>( stream ); }

}; //namespace Parsec;
