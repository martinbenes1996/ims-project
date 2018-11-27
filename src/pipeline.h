
/**
 * @file pipeline.h
 * @interface pipeline
 * @authors xbenes49 xpolan09
 * @date 27th november 2018
 * @brief Pipeline classes interface.
 *
 * This interface declares Source, Pipe and Pipeline classes and its relatives.
 */

#ifndef PIPELINE_H
#define PIPELINE_H

#include <map>
#include <string>

#include "simlib.h"

#include "tools.h"

/* ------------------------------------------------------------------------------------ */
/** @addtogroup Source
 * Source class.
 * @{
 */

/**
 * @brief Source of oil.
 */
class Source : public Process {
    public:
        /**
         * @brief Constructor.
         * @param name          Name (for printing).
         * @param production    Initial production.
         * @param output        Output callback.
         */
        Source(std::string name, double production, Callback output):
            mname(name), mproduction(production), moutput(output) {}
        
        /**
         * @brief Overriden function, called through event in calendar.
         */
        void Behavior();

        /**
         * @brief Current production setter.
         * @param p         New production value.
         */
        void setProduction(double p) { mproduction = p;}
        /**
         * @brief Current production getter.
         * @returns Current production.
         */
        double getProduction() { return mproduction; }

    private:
        std::string mname; /**< Name of source. */
        double mproduction; /**< Current production. */
        Callback moutput; /**< Output callback function. */
};

/** @}*/
/* ------------------------------------------------------------------------------------ */
/** @addtogroup Pipe
 * Pipe and Transfer class.
 * @{
 */

/**
 * @brief Transaction of pipe.
 */
class Transfer: public Process {
    public:
        /**
         * @brief Constructor.
         * @param amount        Amount.
         * @param output        Callback.
         */
        Transfer(double amount, Callback output):
            mamount(amount), moutput(output) {
            #ifdef TRANSFER_LOG
                std::cerr << Time << ") Transfer of " << mamount << " initialized.\n";
            #endif
        }

        /**
         * @brief Overriden function, called through event in calendar.
         */
        void Behavior();

    private:
        double mamount; /**< Amount of oil. */
        Callback moutput; /**< Output callback. */
};

/**
 * @brief Pipe class. Delayed connection.
 */
class Pipe {
    public:
        /**
         * @brief Constructor.
         * @param name          Name (for printing).
         * @param maximum       Maximum.
         * @param delay         Delay.
         * @param output        Callback.
         */
        Pipe(std::string name, double maximum, double delay, Callback output=[](double){ std::cerr << "Output not set!\n"; }):
            mname(name), il(maximum), d(delay), moutput(output) {}

        /**
         * @brief Sends amount through pipe.
         * @param amount        Amount to send.
         */
        void Send(double);
        /**
         * @brief Input getter.
         * @returns Input callback.
         */
        Callback getInput() { return [this](double amount){ this->Send(amount);}; }

        /**
         * @brief Output setter.
         * @param output        New output.
         */
        void setOutput(Callback output) { moutput = output; }
        /**
         * @brief Output getter.
         * @returns Output callback.
         */
        Callback getOutput() {
            return [this](double amount){
                sending.erase(Time);
                return this->moutput(amount);
            };
        }

        /** @brief Breaks the pipe. */
        void Break() { f.Set(); }
        /** @brief Fixes broken pipe. */
        void Fix() { f.Reset(); }
        /** @brief Broken indicator. */
        bool IsBroken() { return f.IsSet(); }

        /**
         * @brief Current flow getter.
         * @returns Map of time and amount coming.
         */
        std::map<double,double> getCurrentFlow();
        /**
         * @brief Plan setter. Used during the initialization.
         * @param s         New map of times and amount.
         */
        void setSending(std::map<double,double> s) { sending = s; }

    protected:
        /**
         * @brief Sending planner.
         * @param amount        Amount to send.
         * @param t             Initial time.
         */
        void PlanSending(double, double);

    private:
        std::string mname; /**< Name. */
        InputLimiter il; /**< Limitter. */
        double d; /**< Delay. */
        Callback moutput; /**< Output callback. */

        double maxStorage = 100; /**< Maximal storage. */
        std::map<double, double> sending; /**< Plan for sending. */

        Flagger f; /**< Broken indicator. */
};

/** @}*/
/* ------------------------------------------------------------------------------------ */
/** @addtogroup Pipeline
 * Pipeline class and relatives
 * @{
 */

/**
 * @brief Status of pipeline.
 */
struct PipelineStatus {
    std::string name; /**< Name. */
    std::map<double,double> delivery; /**< Delivery plan.  */
    double production; /**< Current production. */
    double maximum; /**< Maximal possible production. */
    bool broken; /**< Broken flag. */

    /** @brief Prints brief output. */
    void print();
};

/**
 * @brief Represents pipeline.
 */
class OilPipeline {
    public:
        /**
         * @brief Constructor.
         * @param name              Name (for printing).
         * @param maxProduction     Maximum production.
         * @param producing         Initial producing amount.
         * @param delay             Delay of deliveries.
         */
        OilPipeline(std::string, double, double, double);

        /**
         * @brief Output callback getter.
         * @returns Output callback.
         */
        Callback getOutput() { return [this](double amount){ this->Output(amount); }; }

        /** @brief Broken indicator. */
        bool IsBroken() { return p->IsBroken(); }
        /** @brief Breaks pipeline. */
        void Break() { p->Break(); }
        /** @brief Fixes pipeline. */
        void Fix() { p->Fix(); }

        /**
         * @brief Output callback setter.
         * @param output        New output value.
         */
        void setOutput(Callback output) { moutput = output; }
        /**
         * @brief Production setter.
         * @param production    New production value.
         */
        void setProduction(double production) { s->setProduction(production); }

        /**
         * @brief Status getter.
         * @returns Status of the pipeline.
         */
        PipelineStatus getStatus();
        

    private:
        std::string mname; /**< Pipeline name. */
        double mmaximum; /**< Maximum possible flow. */
        double mdelay; /**< Delay of pipeline. */

        Callback moutput; /**< Callback output. */
        /** @brief Output. */
        void Output(double);

        Pipe* p; /**<  Pipe object. */
        Source* s; /**< Source object. */
};

/** @}*/
/* ------------------------------------------------------------------------------------ */

#endif // PIPELINE_H