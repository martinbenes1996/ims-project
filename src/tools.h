#ifndef TOOLS_H
#define TOOLS_H

#include <algorithm>
#include <cstring>
#include <functional>
#include <iostream>
#include <sstream>
#include <vector>

// log compile constants
#ifdef OUTPUT_LOG
    //#define SOURCE_LOG
    //#define PIPES_LOG
    //#define FLAGGER_LOG
    //#define SIMULATOR_LOG
    //#define RAFINERY_LOG
    //#define PIPELINE_LOG
    //#define DISTRIBUTE_LOG
    //#define CENTRAL_LOG
    //#define TRANSFER_LOG
    //#define RESERVE_LOG
#endif

/**
 * @brief Converts input string to red output string.
 * @param s         Input.
 * @returns Colored output.
 */
inline std::string red(std::string s) { return std::string("\33[91m") + s + std::string("\33[0m"); }
/**
 * @brief Converts input string to blue output string.
 * @param s         Input.
 * @returns Colored output.
 */
inline std::string blue(std::string s) { return std::string("\33[94m") + s + std::string("\33[0m"); }
/**
 * @brief Converts input string to green output string.
 * @param s         Input.
 * @returns Colored output.
 */
inline std::string green(std::string s) { return std::string("\33[92m") + s + std::string("\33[0m"); }
/**
 * @brief Converts input string to yellow output string.
 * @param s         Input.
 * @returns Colored output.
 */
inline std::string yellow(std::string s) { return std::string("\33[93m") + s + std::string("\33[0m"); }
/**
 * @brief Converts input string to violet output string.
 * @param s         Input.
 * @returns Colored output.
 */
inline std::string violet(std::string s) { return std::string("\33[95m") + s + std::string("\33[0m"); }

/**
 * @brief Converts input string to bold output string.
 * @param s         Input.
 * @returns Styled output.
 */
inline std::string bold(std::string s) { return std::string("\33[1m") + s + std::string("\33[0m"); }
/**
 * @brief Converts input string to italic output string.
 * @param s         Input.
 * @returns Styled output.
 */
inline std::string italic(std::string s) { return std::string("\33[3m") + s + std::string("\33[0m"); }

// text color masks
#define TEXTCOLOR  0x00FF
#define WHITE  0x0000
#define RED    0x0001
#define BLUE   0x0002
#define GREEN  0x0003
#define YELLOW 0x0004
#define VIOLET 0x0005
// text style masks
#define TEXTSTYLE  0xFF00
#define BOLD   0x0100
#define ITALIC 0x0200
#define NORMAL 0x0000
/**
 * @brief Style setter.
 * @param s         Input string.
 * @param format    Format of output (masks, separated by binary OR | ).
 * @returns Formatted output.
 */
inline std::string style(std::string s, int format) {
    // format color
    if((format&TEXTCOLOR) == RED) s = red(s);
    else if((format&TEXTCOLOR) == BLUE) s = blue(s);
    else if((format&TEXTCOLOR) == GREEN) s = green(s);
    else if((format&TEXTCOLOR) == YELLOW) s = yellow(s);
    else if((format&TEXTCOLOR) == VIOLET) s = violet(s);
    // format style
    if((format&TEXTSTYLE) == BOLD) s = bold(s);
    else if((format&TEXTSTYLE) == ITALIC) s = italic(s);

    return s;
}

/**
 * @brief Convertor double > string.
 * @param v         Input double.
 * @returns Result string.
 */
inline std::string double2str(double v) {
    std::ostringstream ss;
    ss << v;
    return ss.str();
}

/**
 * @brief Splits string by whitespaces.
 * @param str           Items in string separated by whitespaces.
 * @returns Vector of strings (items).
 */
inline std::vector<std::string> SplitString(std::string str) {
    std::vector<std::string> v;
    char * s = strdup(str.c_str());
    char * c = strtok(s, " \t");
    while(c != NULL) {
        std::string r = std::string(c);
        std::transform(r.begin(), r.end(), r.begin(), ::tolower);
        v.push_back(r);
        c = strtok(NULL, " \t");
    }
    free(s);
    return v;
}

// Callback function typedef
typedef std::function<void(double)> Callback;

/**
 * @brief Queue for ordering of events.
 */
struct ConsoleFirst {
    Facility waitForConsole; /**< Waiting for console. */
    double t = Time;         /**< Checking, wheather to wait for console or not. */
};
extern struct ConsoleFirst EventOrder; /**< Global instance. */

const double Numeric_Const = 1.0e-03; /**< Epsilon for double counting. */

/**
 * @brief Products of rafineries.
 */
struct Products {
	double benzin = 0;  /**< Benzin. */
	double naphta = 0;  /**< Diesel. */
	double asphalt = 0; /**< Asphalt. */

    /**
     * @brief Operator +=.
     * @param other     Product to add.
     * @returns Result for piping.
     */
    Products& operator+=(const Products& other) {
        this->benzin += other.benzin;
        this->naphta += other.naphta;
        this->asphalt += other.asphalt;
        return *this;
    }
};

// productor function typedef
typedef std::function<void(Products)> Productor;

/**
 * @brief Closing of stream.
 */
class Flagger {
    public:
        /** @brief Constructor. */
        Flagger() {}

        /**
         * @brief Closing of stream.
         * @param amount    Input.
         * @returns Input or 0.
         */
        double Check(double amount) {
            if(mbroken) return 0;
            else return amount;
        }

        /** @brief Close. */
        void Set() { mbroken = true; }
        /** @brief Open. */
        void Reset() { mbroken = false; }
        /** @brief Set. */
        void Set(bool v) { mbroken = !v; }
        /** @brief Status getter. */
        bool IsSet() { return mbroken; }
    private:
        bool mbroken = false; /**< State of flagger. */
};

/**
 * @brief Limits the stream with saturation.
 */
class InputLimiter {
    public:
        /**
         * @brief Constructor.
         * @param maximum       Saturation limit.
         */
        InputLimiter(double maximum): mmaximum(maximum) {}

        /**
         * @brief Saturates the input.
         * @param amount        Input.
         * @param basis         Base, not counted to input.
         * @returns Limited input.
         */
        double output(double amount, double basis=0) { return (amount < (mmaximum-basis))?amount:(mmaximum-basis); }
        /**
         * @brief Saturates the input. Returns the cropped part.
         * @param amount        Input.
         * @param basis         Base, not counted to input.
         * @returns Cropped part of input.
         */
        double rest(double amount, double basis=0) { return (amount < (mmaximum-basis))?0:(amount-mmaximum+basis); }

        /**
         * @brief Maximum getter.
         * @returns Saturation limit.
         */
        double getMaximum() { return mmaximum; }

    private:
        double mmaximum; /**< Saturation limit. */
};

// constants from documentation
const double IKL_Ratio = 0.484863281; /**< IKL Ratio. */
const double Druzba_Ratio = 0.515136718; /**< Druzba Ratio. */
const double Kralupy_Ratio = 0.379353755; /**< Kralupy Ratio. */
const double Litvinov_Ratio = 0.620646244; /**< Litvinov Ratio. */

const double IKL_Max = 27.4; /**< IKL Max. */
const double Druzba_Max = 24.66; /**< Druzba Max. */
const double Kralupy_Max = 9.04; /**< Kralupy Max. */
const double Litvinov_Max = 14.79; /**< Litvinov Max. */

const double Fraction_Benzin = 0.19; /**< Fraction of benzin in crude oil. */
const double Fraction_Naphta = 0.42; /**< Fraction of diesel in crude oil. */
const double Fraction_Asphalt = 0.13; /**< Fraction of diesel in crude oil. */

/**
 * @brief Demand for products.
 */
struct Demand {
    double benzin = 4.38; /**< Demand for benzin. */
    double naphta = 12.96; /**< Demand for diesel. */
    double asphalt = 1.21; /**< Demand for asphalt. */
};

/**
 * @brief Reserve of oil.
 */
class Reserve {
    public:
        /**
         * @brief Constructor.
         * @param name      Name (for printing).
         * @param capacity  Limit.
         */
        Reserve(std::string name, double capacity):
            mname(name), il(capacity), mlevel(capacity) {}

        /**
         * @brief Sends amount to reserve.
         * @param amount        Amount to send.
         * @returns Amount over maximal capacity.
         */
        double Send(double amount) {
            #ifdef RESERVE_LOG
                std::cerr << Time << ") Reserve " << mname << ": Received " << amount <<".\n";
            #endif
            mlevel += il.output(amount);
            return il.rest(amount);
        }

        /**
         * @brief Requests amount from reserve.
         * @param amount        Amount to request.
         * @returns Real delivered amount.
         */
        double Request(double amount) {
            // ignore double inaccuracy
            if(amount<=Numeric_Const) return 0;
            // send only part
            if(mlevel < amount) {
                amount = mlevel;
                mlevel = 0;
                return amount;
            // enough of reserve - send all
            } else {
                mlevel -= amount;
                return amount;
            }
        }

        /**
         * @brief Capacity getter.
         * @returns Capacity of reserve.
         */
        double getCapacity() { return il.getMaximum(); }
        /**
         * @brief Oil level getter.
         * @returns Current level of oil in reserve.
         */
        double Level() { return mlevel; }

        /**
         * @brief Computes how much is missing to meet EU regulation.
         * @returns 0 if ok, or missing amount.
         */
        double Missing() {
            double miss = mlevel - 900.0;
            if(miss >= 0.0) miss = 0.0;
            return -miss;
        }

    private:
        std::string mname; /**< Name. */
        InputLimiter il;   /**< Limiter of input. */
        double mlevel;     /**< Current level. */
};

#endif // TOOLS_H
