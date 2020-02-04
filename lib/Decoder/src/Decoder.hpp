#ifndef DECODER_HPP_
#define DECODEER_HPP_

#include <iostream>
#include <map>

class Decoder
{
public:
    Decoder(void);
    std::string decode(std::string code);
    uint16_t size();

private:
    std::map<std::string, std::string> letter;
};

#endif