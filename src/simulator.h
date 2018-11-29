
/**
 * @file simulator.h
 * @interface simulator
 * @authors xbenes49 xpolan09
 * @date 27th november 2018
 * @brief Simulator class interface.
 *
 * This interface declares Simulator class.
 */

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "central.h"
#include "pipeline.h"
#include "rafinery.h"
#include "tools.h"

/* ------------------------------------------------------------------------------------ */
/** @addtogroup Simulator
 * Simulator class.
 * @{
 */

/**
 * @brief Class Simulator.
 *
 * Simulator instatiates main parts of system, connects them and hold them. It also implements terminal.
 */
class Simulator: public Process {
    public:

        /**
         * @brief Constructor. Instatiates parts of system and connects them.
         */
        Simulator();


        /**
         * @brief Handler of products from rafineries.
         */
        void AcquireProducts(Products p) { mproducts += p; }
        /**
         * @brief Productor getter.
         */
        Productor getProductor() { return [this](Products p){ return this->AcquireProducts(p); }; }
        void ResolveDayDemand() {
            double benzin = normalize(mproducts.benzin, demand.benzin - import.benzin) - (demand.benzin - import.benzin);
            double naphta = normalize(mproducts.naphta, demand.naphta - import.naphta) - (demand.naphta - import.naphta);
            double asphalt = normalize(mproducts.asphalt, demand.asphalt - import.asphalt) - (demand.asphalt - import.asphalt);
            std::string benzinS = double2str(benzin), naphtaS = double2str(naphta), asphaltS = double2str(asphalt);
            benzinS = (benzin < 0) ? red(benzinS) : ((benzin > 0) ? green(benzinS) : benzinS );
            naphtaS = (naphta < 0) ? red(naphtaS) : ((naphta > 0) ? green(naphtaS) : naphtaS );
            asphaltS = (asphalt < 0) ? red(asphaltS) : ((asphalt > 0) ? green(asphaltS) : asphaltS );

            double oilNeed = max_3((demand.benzin-import.benzin)/Fraction_Benzin, (demand.naphta-import.naphta)/Fraction_Naphta, (demand.asphalt-import.asphalt)/Fraction_Asphalt);
            std::cout << oilNeed << " of oil needed.\n";
            std::cout << bold("Demand satisfaction:\n");

            if(oilNeed>((Kralupy->IsBroken())?0:Kralupy_Max) + ((Litvinov->IsBroken())?0:Litvinov_Max)) 
                std::cout << red("Demand is too high and cannot be satisfied with current refineries!\n");

            std::cout << italic("\t- benzin\t") << benzinS << "\n";
            std::cout << italic("\t- naphta\t") << naphtaS << "\n";
            std::cout << italic("\t- asphalt\t") << asphaltS << "\n";
            mproducts = Products();

            ReserveStatus rs = CTR->getStatus();
            std::cout << bold("Reserve ") << bold(rs.name) << bold(" balance:\n");
            std::cout << rs.level << "/" << rs.capacity << "\n";
            if(rs.requested != -1) {
                std::cout << "Taken " << red( double2str(rs.given) ) << " [" << italic( double2str(rs.requested) ) << " requested]\n";
            }
            if(rs.added != -1) {
                std::cout << "Added " << green( double2str(rs.added) ) << " [" << italic( double2str(rs.returned) ) << " requested]\n";
            }
            std::cout << "\n";
        }


        /**
         * @brief Behavior of process. Overriden method, called by calendar.
         */
        void Behavior() { TerminalLoop(); }
        /**
         * @brief Implementation of terminal (input and output).
         */
        void TerminalLoop();


    private:
        /**
         * @brief Counts absolute value of two numbers.
         * @param number        Number.
         * @returns Not a negative number.
         */
        double absolutni(double number) {return (number>=0.0)?number:-number;}
        /**
         * @brief If there is just a small difference between two numbers, it is neglected.
         * @param normalized        Number that is being changed.
         * @param normalizing       Target value.
         * @returns "normalized" number if the difference is too big, "normalizing" number if the difference is small enough.
         */
        double normalize(double normalized, double normalizing) {
            if(absolutni(normalized-normalizing)<Numeric_Const || absolutni(normalized-normalizing)>-Numeric_Const)
                normalized = normalizing;
            return normalized;
        }
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
        // parts of system
        // pipelines
        OilPipeline* Druzba; /**< Druzba. Connected to Central. */
        OilPipeline* IKL; /**< IKL. Connected to Central. */
        // rafineries
        Rafinery* Kralupy; /**< Kralupy. Connected directly to Central. */
        Rafinery* Litvinov; /**< Litvinov. Connected by pipe to Central. */
        Pipe* Cent_Litvinov_Pipe; /**< Pipe Litvinov <> Central. */
        // reserves
        Reserve* CTR; /**< Reserve at Nelahozeves. Connected directly to Central. */
        // central
        Central* CentralaKralupy; /**< Central at Kralupy. */

        // inputs and output
        // inputs
        Demand demand; /**< Current demand. */
        Import import; /**< Current import. */
        // outputs
        Products mproducts; /**< Current production. */

};

/** @}*/
/* ------------------------------------------------------------------------------------ */

#endif // SIMULATOR
