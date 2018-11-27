
/**
 * @file central.h
 * @interface central
 * @authors xbenes49 xpolan09
 * @date 27th november 2018
 * @brief Central classes interface.
 *
 * This interface declares Central class and its relatives.
 */

#ifndef CENTRAL_H
#define CENTRAL_H

#include "pipeline.h"
#include "rafinery.h"
#include "tools.h"

/**
 * @brief Input Ratio.
 */
struct CentInputRatio {
    double IKL = IKL_Ratio; /**< Current IKL Ratio. */
    double Druzba = Druzba_Ratio; /**< Current Druzba Ratio. */
};
/**
 * @brief Output Ratio.
 */
struct CentOutputRatio {
    double Kralupy = Kralupy_Ratio; /**< Current Kralupy Ratio. */
    double Litvinov = Litvinov_Ratio; /**< Current Litvinov Ratio. */
    double Reserve = 0.0;
};
/**
 * @brief Hearth of the model, performs most of its logic. Distributes oil to refineries, plans order for pipelines.
 */
class Central {
    public:
        /**
         * @brief Constructor.
         * @param kr                    Kralupy refinery.
         * @param lit                   Litvinov refinery.
         * @param druzba                Druzba pipeline.
         * @param ikl                   IKL pipeline.
         * @param Cent_Litvinov_Pipe    Pipe to Litvinov refinery.
         * @param ctr                   Nelahozeves reserve.
         * @param d                     Current demand structure.
         */
        Central(Rafinery *kr, Rafinery *lit, OilPipeline* druzba, OilPipeline* ikl, Pipe* Cent_Litvinov_Pipe, Reserve* ctr, Demand& d):
            Kralupy(kr), Litvinov(lit), Druzba(druzba), IKL(ikl), LitPipe(Cent_Litvinov_Pipe), CTR(ctr), demand(d)
            {
                koutput = Kralupy->getInput();
                loutput = LitPipe->getInput();
            }
        /**
         * @brief Distributes and orders oil.
         * @param amount        Oil amount.
         */
        void Enter(double amount) {
            // check for disasters: something is broken -> 0 + the rest gets 1
            if(!Druzba->IsBroken() && !IKL->IsBroken()) {           // if everything is normal (99% of all checks)
                input.Druzba = Druzba_Ratio;
                input.IKL = IKL_Ratio;
            }
            else if(Druzba->IsBroken() && IKL->IsBroken()) {
                input.Druzba = 0;
                input.IKL = 0;
            }
            else if(Druzba->IsBroken()) {
                input.Druzba = 0;
                input.IKL = 1;
            }
            else {
                input.Druzba = 1;
                input.IKL = 0;
            }

            if(!Kralupy->IsBroken() && !Litvinov->IsBroken()) {     // if everything is normal (99% of all checks)
                output.Kralupy = Kralupy_Ratio;
                output.Litvinov = Litvinov_Ratio;
            }
            else if(Kralupy->IsBroken() && Litvinov->IsBroken()) {
                output.Kralupy = 0;
                output.Litvinov = 0;
            }
            else if(Kralupy->IsBroken()) {
                output.Kralupy = 0;
                output.Litvinov = 1;
            }
            else {
                output.Kralupy = 1;
                output.Litvinov = 0;
            }

            // central receives up to two deliveries of oil per day
            // if it is a new day (first delivery), remember the amount of oil received and wait for more
            if(firstDelivery) {
                oilToday = amount;
                firstDelivery = false;
                return;                 // wait for second load of oil
            }
            else {
                oilToday += amount;
                firstDelivery = true;
                // count demand for today
                demandOil = max_3(demand.benzin/Fraction_Benzin, demand.naphta/Fraction_Naphta, demand.asphalt/Fraction_Asphalt);
            }
            // correction of oil amount to fit demand - DEMAND FIRST, RESERVE SECOND
            short KralupyBreakFlag;     /**< 0 when broken, 1 when ok. */
            short LitvinovBreakFlag;    /**< 0 when broken, 1 when ok. */
            KralupyBreakFlag = (Kralupy->IsBroken())?0:1;
            LitvinovBreakFlag = (Litvinov->IsBroken())?0:1;
            // if there is not enough oil in central
            if(demandOil > oilToday && (demandOil-oilToday > Numeric_Const)) {
                // ask reserve for oil (only as much as the refineries will be able to process)
                if(demandOil <= Kralupy_Max*KralupyBreakFlag + Litvinov_Max*LitvinovBreakFlag) {
                    #ifdef DISTRIBUTE_LOG
                        std::cerr << Time << ") Central: Asking reserve for " << demandOil-oilToday << " units of oil.\n";
                    #endif
                    oilToday += CTR->Request(demandOil-oilToday);
                }
                else {
                    #ifdef DISTRIBUTE_LOG
                        double help = ((Kralupy_Max*KralupyBreakFlag+Litvinov_Max*LitvinovBreakFlag)-oilToday>0.0)?(Kralupy_Max*KralupyBreakFlag+Litvinov_Max*LitvinovBreakFlag)-oilToday:0.0;
                        if(help<Numeric_Const) help = 0.0;
                        std::cerr << Time << ") Central: Asking reserve for " << help << " units of oil. Demand too high.\n";
                    #endif
                    oilToday += CTR->Request((Kralupy_Max*KralupyBreakFlag+Litvinov_Max*LitvinovBreakFlag)-oilToday);
                }
                #ifdef CENTRAL_LOG
                    if(oilToday < demandOil || (Kralupy_Max*KralupyBreakFlag+Litvinov_Max*LitvinovBreakFlag)<demandOil)
                        std::cerr << Time << ") Central: Demand cannot be satisfied today, " << demandOil-oilToday << " units of oil are missing.\n";
                #endif
            }
            // if there is too much oil in central
            else {
                // send oil to reserve
                double missing = CTR->Missing();        // how much oil is missing in reserve to ideal
                double canSend = oilToday - demandOil;  // how much oil can be sent but still satisfy demand
                if(missing != 0.0 && canSend != 0.0) {
                    #ifdef DISTRIBUTE_LOG
                        std::cerr << Time << ") Central: Sending " << ((missing<=canSend)?missing:canSend) << " to CTR\n";
                    #endif
                    // send all that is over demand ???
                    // send up to canSend value or full missing chunk
                    CTR->Send((missing<=canSend)?missing:canSend);
                    // update the amount of oil in central
                    oilToday = (missing<=canSend)?oilToday-missing:oilToday-canSend;
                    //double returnOil;
                    //returnOil = CTR->Send(canSend);
                    //oilToday -= (canSend-returnOil);
                }
            }
            double overflow = 0.0;      /**< Stores oil that could not fit in refinery. */

            if(oilToday*output.Kralupy > Kralupy_Max) {
                koutput(Kralupy_Max);
                overflow += oilToday*output.Kralupy - Kralupy_Max;
            }
            else
                koutput(oilToday*output.Kralupy);
            #ifdef DISTRIBUTE_LOG
                std::cerr << Time << ") Central: Sending " << oilToday*output.Kralupy-overflow << " to Kralupy.\n";
            #endif

            if(oilToday*output.Litvinov > Litvinov_Max) {
                loutput(Litvinov_Max);
                overflow += oilToday*output.Litvinov - Litvinov_Max;
            }
            else
                loutput(oilToday*output.Litvinov);
            #ifdef DISTRIBUTE_LOG
                double x = (oilToday*output.Litvinov > Litvinov_Max)?Litvinov_Max:(oilToday*output.Litvinov);
                std::cerr << Time << ") Central: Sending " << x << " to Litvinov.\n";
            #endif

            // if both refineries are broken, send oil to reserve
            if(!output.Kralupy && !output.Litvinov)
                overflow = oilToday;
            #ifdef DISTRIBUTE_LOG
                std::cerr << Time << ") Central: Sending " << overflow << " to CTN.\n";
            #endif
            double lostOil;     /**< Amount of lost oil. If oil cannot be sent to refineries or reserve, its gone. */
            (void)lostOil;
            lostOil = CTR->Send(overflow);
            #ifdef DISTRIBUTE_LOG
                if(lostOil)
                    std::cerr << Time << ") Central: " << lostOil << " units of oil lost.\n";
            #endif

            double totalNeed;   /**< Total need of oil for the next days. Based on current demand. */
            // ignores travel time -> will give reserve more than necessary, which is fine
            double Kralupy_req = (output.Kralupy==0.0)?0.0:Kralupy_Max;     /**< Hunger of Kralupy refinery. */
            double Litvinov_req = (output.Litvinov==0.0)?0.0:Litvinov_Max;  /**< Hunger of Litvinov refinery. */
            // if demand is higher than the capacity of both refineries, request max capacity, else request demand
            // add missing amount of oil in reserve
            totalNeed = (demandOil>(Kralupy_req+Litvinov_req))?(Kralupy_req+Litvinov_req):demandOil + CTR->Missing();
            // request for oil pipelines
            // if one oilPipeline is full, second one shares the burden
            // if both pipelines are overburdened, they both send up to maximum but no more (logic in OilPipeline class)
            double toIKL = 0.0;             /**< Shift oil demand to ikl. */
            double toDruzba = 0.0;          /**< Shift oil demand to druzba. */
            if(totalNeed*input.Druzba > Druzba_Max) toIKL = totalNeed*input.Druzba - Druzba_Max;
            if(totalNeed*input.IKL > IKL_Max) toDruzba = totalNeed*input.IKL - IKL_Max;
            #ifdef CENTRAL_LOG
                std::cerr << Time << ") Central: Setting production for Druzba to " << totalNeed*input.Druzba + toDruzba << ".\n";
            #endif
            Druzba->setProduction(totalNeed*input.Druzba + toDruzba);
            #ifdef CENTRAL_LOG
                std::cerr << Time << ") Central: Setting production for IKL to " << totalNeed*input.IKL + toIKL << ".\n";
            #endif
            IKL->setProduction(totalNeed*input.IKL + toIKL);

        }

        /**
         * @brief Input callback getter.
         */
        Callback getInput() { return [this](double amount){this->Enter(amount);}; }

        /**
         * @brief Update current demand.
         * @param d        Current demand.
         */
        void setDemand(struct Demand& d) { demand = d; }

    private:
        /**
         * @brief Returns the maximum of three numbers.
         * @param b        First number.
         * @param n        Second number.
         * @param a        Third number.
         * @returns The maximal number.
         */
        double max_3(double b, double n, double a) {
            double maximum = (b>n)?b:n;
            return (maximum>a)?maximum:a;
        }
        /**< Broken flag. */
        struct CentInputRatio input;        /**< Current ratio of input - negotiate with OilPipelines. */
        struct CentOutputRatio output;      /**< Current ratio of output - negotiate with Rafinery. */
        Callback koutput;                   /**< Output of central - input function of Kralupy. */
        Callback loutput;                   /**< Output of central - input function of Litvinov. */
        Rafinery *Kralupy;                  /**< Kralupy refinery. */
        Rafinery *Litvinov;                 /**< Litvinov refinery. */
        OilPipeline* Druzba;                /**< Druzba pipeline. */
        OilPipeline* IKL;                   /**< IKL pipeline. */
        Pipe* LitPipe;                      /**< Oil for Litvinov is sent via this pipe. */
        Reserve* CTR;                       /**< Reserve of oil. */
        struct Demand& demand;              /**< Current demand set by simulator. */
        bool firstDelivery = true;          /**< First delivery of this day. */
        double oilToday = 0;                /**< Oil received today so far. */
        double demandOil = 0;               /**< Demand of oil for today. */
};

#endif // CENTRAL_H
