# Parsec
Your worst nightmare regarding C++ template metaprogramming)

## Ah yes, the parser... 
> Parser creation:
```C++
auto parser = make_parser("([" << repeat<3>(value<int>() << ",")
                          << "] " << value<std::string>().then(")") );
```
The parsers are built using 'schemes':
- `value<T>(...)` [ tells the parser to extract the value of type T ]
- `match(...)`    [ tells the parser to match argument of match()   ]
- `repeat<N>( scheme )` [ returns an std::array<scheme::ret_type, N>]
- `Chain<Scheme1, Scheme2>` (cast implicitly when using scheme1 << scheme2
- "..." gets converted to match("...") when used in << with another scheme
- any combination of the above

> Actual parsing (using C++17's new syntax) 
```C++ 
auto [x,y] = parser.parse("([11,22,33,] a string)");
```
