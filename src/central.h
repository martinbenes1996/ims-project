
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

class Central {
    public:
    #warning Zdokumentuj si central
        Central(Rafinery *kr, Rafinery *lit, OilPipeline* druzba, OilPipeline* ikl, Pipe* Cent_Litvinov_Pipe, Reserve* ctr, Demand& d):
            Kralupy(kr), Litvinov(lit), Druzba(druzba), IKL(ikl), LitPipe(Cent_Litvinov_Pipe), CTR(ctr), demand(d)
            {
                koutput = Kralupy->getInput();
                loutput = LitPipe->getInput();
            }
        void Enter(double amount) {
            // check for disasters
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
                //output.Reserve = 0.0;
            }
            else if(Kralupy->IsBroken() && Litvinov->IsBroken()) {
                output.Kralupy = 0;
                output.Litvinov = 0;
                //output.Reserve = 1;
            }
            else if(Kralupy->IsBroken()) {
                output.Kralupy = 0;
                output.Litvinov = 1;
                //output.Reserve = Kralupy_Ratio;
            }
            else {
                output.Kralupy = 1;
                output.Litvinov = 0;
                //output.Reserve = Litvinov_Ratio;
            }

            // central receives up to two deliveries of oil per day
            // if it is a new day (first delivery), remember the amount of oil received and wait for more
            if(firstDelivery) {
                //
                oilToday = amount;
                firstDelivery = false;
                return;
            }
            else {
                oilToday += amount;
                firstDelivery = true;
                demandOil = max_3(demand.benzin/Fraction_Benzin, demand.naphta/Fraction_Naphta, demand.asphalt/Fraction_Asphalt);
            }
            // demand correction - DEMAND FIRST, RESERVE SECOND
            short KralupyBreakFlag;
            short LitvinovBreakFlag;
            KralupyBreakFlag = (Kralupy->IsBroken())?0:1;
            LitvinovBreakFlag = (Litvinov->IsBroken())?0:1;
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
            else {
                // reserve interactions - send
                double missing = CTR->Missing();
                double canSend = oilToday - demandOil;
                if(missing != 0.0 && canSend != 0.0) {
                    #ifdef DISTRIBUTE_LOG
                        std::cerr << Time << ") Central: Sending " << ((missing<=canSend)?missing:canSend) << " to CTR\n";
                    #endif
                    // send all that is over demand ???
                    CTR->Send((missing<=canSend)?missing:canSend);
                    oilToday = (missing<=canSend)?oilToday-missing:oilToday-canSend;
                    //double returnOil;
                    //returnOil = CTR->Send(canSend);
                    //oilToday -= (canSend-returnOil);
                }
            }
            double overflow = 0.0;

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

            if(!output.Kralupy && !output.Litvinov)
                overflow = oilToday;
            #ifdef DISTRIBUTE_LOG
                std::cerr << Time << ") Central: Sending " << overflow << " to CTN.\n";
            #endif
            double lostOil;
            (void)lostOil;
            lostOil = CTR->Send(overflow);
            #ifdef DISTRIBUTE_LOG
                if(lostOil)
                    std::cerr << Time << ") Central: " << lostOil << " units of oil lost.\n";
            #endif

            // TODO: reflektovat potrebu ropy - vyslat pozadavek na ropovody (missing,demand,oilToday,input)
            double totalNeed;
            // ignores travel time -> will give reserve more than necessary (should not be a problem)
            // can be improved by mapping requests...
            double Kralupy_req = (output.Kralupy==0.0)?0.0:Kralupy_Max;
            double Litvinov_req = (output.Litvinov==0.0)?0.0:Litvinov_Max;
            totalNeed = (demandOil>(Kralupy_req+Litvinov_req))?(Kralupy_req+Litvinov_req):demandOil + CTR->Missing();
            // request for oil pipelines
            // if one oilPipeline is full, second one shares the burden
            double toIKL = 0.0;             // shift oil demand to ikl
            double toDruzba = 0.0;          // shift oil demand to druzba
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

        Callback getInput() { return [this](double amount){this->Enter(amount);}; }

        void setDemand(struct Demand& d) { demand = d; }

        void Behavior() {}
    private:
        double max_3(double b, double n, double a) {
            double maximum = (b>n)?b:n;
            return (maximum>a)?maximum:a;
        }
        struct CentInputRatio input;        // current ratio of input - negotiate with OilPipelines
        struct CentOutputRatio output;      // current ratio of output - negotiate with Rafinery
        Callback koutput;                   // output of central - input function of Kralupy
        Callback loutput;                   // output of central - input function of Litvinov
        Rafinery *Kralupy;
        Rafinery *Litvinov;
        OilPipeline* Druzba;
        OilPipeline* IKL;
        Pipe* LitPipe;                      // oil for Litvinov is sent via this pipe
        Reserve* CTR;
        struct Demand& demand;              // current demand set by simulator
        //int lastTime = -1;                  // time of the last call of enter function
        bool firstDelivery = true;          // first delivery of this day
        double oilToday = 0;                // oil received today so far
        double demandOil = 0;               // demand of oil for today
};

#endif // CENTRAL_H