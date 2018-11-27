#ifndef LOG_H
#define LOG_H

#include <functional>
#include <sstream>

#ifdef OUTPUT_LOG
    //#define SOURCE_LOG
    //#define PIPES_LOG
    //#define FLAGGER_LOG
    //#define SIMULATOR_LOG
    //#define RAFINERY_LOG
    //#define PIPELINE_LOG
    #define DISTRIBUTE_LOG
    #define CENTRAL_LOG
    //#define TRANSFER_LOG
    #define RESERVE_LOG
#endif

std::string red(std::string s) { return std::string("\33[91m") + s + std::string("\33[0m"); }
std::string blue(std::string s) { return std::string("\33[94m") + s + std::string("\33[0m"); }
std::string green(std::string s) { return std::string("\33[92m") + s + std::string("\33[0m"); }
std::string yellow(std::string s) { return std::string("\33[93m") + s + std::string("\33[0m"); }
std::string violet(std::string s) { return std::string("\33[95m") + s + std::string("\33[0m"); }

std::string bold(std::string s) { return std::string("\33[1m") + s + std::string("\33[0m"); }
std::string italic(std::string s) { return std::string("\33[3m") + s + std::string("\33[0m"); }

#define TEXTCOLOR  0x00FF
#define WHITE  0x0000
#define RED    0x0001
#define BLUE   0x0002
#define GREEN  0x0003
#define YELLOW 0x0004
#define VIOLET 0x0005

#define TEXTSTYLE  0xFF00
#define BOLD   0x0100
#define ITALIC 0x0200
#define NORMAL 0x0000

std::string style(std::string s, int format) {
    if((format&TEXTCOLOR) == RED) s = red(s);
    else if((format&TEXTCOLOR) == BLUE) s = blue(s);
    else if((format&TEXTCOLOR) == GREEN) s = green(s);
    else if((format&TEXTCOLOR) == YELLOW) s = yellow(s);
    else if((format&TEXTCOLOR) == VIOLET) s = violet(s);

    if((format&TEXTSTYLE) == BOLD) s = bold(s);
    else if((format&TEXTSTYLE) == ITALIC) s = italic(s);

    return s;
}

std::string double2str(double v) {
    std::ostringstream ss;
    ss << v;
    return ss.str();
}

#endif // LOG_H
