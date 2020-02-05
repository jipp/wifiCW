#ifndef CW_DECODER_HPP_
#define CW_DECODEER_HPP_

#include <string>
#include <map>

class CWDecoder
{
public:
    CWDecoder(void);
    std::string decode(std::string code);
    uint16_t size();

private:
    std::map<std::string, std::string> letter;
};

#endif