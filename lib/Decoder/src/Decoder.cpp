#include "Decoder.hpp"

Decoder::Decoder(void)
{
    letter["._"] = "a";
    letter["_..."] = "b";
    letter["_._."] = "c";
    letter["_.."] = "d";
    letter["."] = "e";
    letter[".._."] = "f";
    letter["__."] = "g";
    letter["...."] = "h";
    letter[".."] = "i";
    letter[".___"] = "j";
    letter["_._"] = "k";
    letter["._.."] = "l";
    letter["__"] = "m";
    letter["_."] = "n";
    letter["___"] = "o";
    letter[".__."] = "p";
    letter["__._"] = "q";
    letter["._."] = "r";
    letter["..."] = "s";
    letter["_"] = "t";
    letter[".._"] = "u";
    letter["..._"] = "v";
    letter[".__"] = "w";
    letter["_.._"] = "x";
    letter["_.__"] = "y";
    letter["__.."] = "z";
    letter[".____"] = "1";
    letter["..___"] = "2";
    letter["...__"] = "3";
    letter["...._"] = "4";
    letter["....."] = "5";
    letter["_...."] = "6";
    letter["__..."] = "7";
    letter["___.."] = "8";
    letter["____."] = "9";
    letter["_____"] = "0";
    // Sonderzeichen
    letter["_._._"] = "KA";
    letter["_..._"] = "BT";
    letter["._._."] = "AR";
    letter["..._."] = "VE";
    letter["..._._"] = "SK";
    letter["...___..."] = "SOS";
    letter["........"] = "HH";
}

std::string Decoder::decode(std::string code)
{
    if (letter.find(code) == letter.end())
        return "?";

    return letter[code];
}

uint16_t Decoder::size()
{
    return letter.size();
}