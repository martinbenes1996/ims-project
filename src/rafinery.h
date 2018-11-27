#ifndef RAFINERY_H
#define RAFINERY_H

#include <iostream>
#include <map>
#include <string>

#include "simlib.h"

#include "tools.h"

/**
 * @brief Status of rafinery.
 */
struct RafineryStatus {
    std::string name; /**< Name of the rafinery. */
    std::map<double,double> production; /**< Production in <Time,Time+d>. */
    double maximum; /**< Maximum. */
    bool broken; /** Broken flag. */

    /** @brief Prints brief output. */
    void print();
};

/**
 * @brief Transaction of Rafinery.
 */
class FractionalDestillation: public Event {
    public:
        /** @brief Constructor. */
        FractionalDestillation(double amount, Productor output):
            mamount(amount), moutput(output) {}
        
        /**
         * @brief Overriden method called by calendar on event.
         */
        void Behavior() {
            moutput( Destillate(mamount) );
        }
        /**
         * @brief Performs destillation. Returns products.
         * @param amount        Oil amount.
         * @returns Products structure.
         */
        Products Destillate(double);

    private:
        double mamount; /**< Amount of oil. */
        Productor moutput; /** Productor callback function. */
};


/**
 * @brief Rafinery class.
 */
class Rafinery: public Event {
    public:
        /**
         * @brief Constructor.
         * @param name          Name (for printing).
         * @param maxProcessing Maximum of single transaction.
         */
        Rafinery(std::string name, double maxProcessing, double delay):
            mname(name), il(maxProcessing), d(delay) {}
        
        /**
         * @brief Handles oil and process it.
         * @param amount        Amount to process.
         */
        void Enter(double);
        /**
         * @brief Input callback getter.
         */
        Callback getInput() { return [this](double amount){ this->Enter(amount); }; }

        /**
         * @brief Overriden method, called via event from calendar.
         */
        void Behavior() {
            #warning output here!
            //moutput(processing.at(Time));
            #ifdef RAFINERY_LOG
                std::cerr << Time << ") Rafinery " << mname << ": Processed " << processing.at(Time) << "\n";
            #endif
            processing.erase(Time);
        }

        /** @brief Indicator of being broken. */
        bool IsBroken() { return f.IsSet(); }
        /** @brief Break setter. */
        void Break() { f.Set(); }
        /** @brief Break resetter. */
        void Fix() { f.Reset(); }

        /**
         * @brief Output of rafinery (prints).
         * @param p         Output products.
         */
        void output(Products p) {
            #ifdef RAFINERY_LOG
                std::cerr << Time << ") Rafinery " << mname << ": Processed ["<<"b:"<<p.benzin<<", "
                                                                              <<"n:"<<p.naphta<<", "
                                                                              <<"a:"<<p.asphalt<<"].\n";
            #endif
            mproductor(p);
        }
        /**
         * @brief Output callback getter. */
        Productor getOutput() { return [this](Products p){this->output(p);}; }
        /** @brief Productor setter. */
        void setProductor(Productor p) { mproductor = p; }

        /**
         * @brief Status getter.
         * @returns Status.
         */
        RafineryStatus getStatus();
        /**
         * @brief Production getter,
         * @returns Production in <Time, Time+d>.
         */
        std::map<double,double> getProduction();

    private:
        std::string mname; /**< Name. */
        InputLimiter il; /**< Limiter of input. */
        double d; /**< Time delay. */
        Flagger f; /**< Broken flag. */

        double maxStorage = 100; /**< Storage limit (constant). */
        std::map<double, double> processing; /**< Processing plan. */
        /**
         * @brief Processing planner.
         * @param amount        Amount to plan.
         * @param t             Begin time.
         */
        void PlanProcessing(double, double);

        Productor mproductor; /**< Output productor. */
};

#endif // RAFINERY_H