
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
            double benzin = mproducts.benzin - demand.benzin;
            double naphta = mproducts.naphta - demand.naphta;
            double asphalt = mproducts.asphalt - demand.asphalt;
            std::string benzinS = double2str(benzin), naphtaS = double2str(naphta), asphaltS = double2str(asphalt);
            benzinS = (benzin < 0) ? red(benzinS) : ((benzin > 0) ? green(benzinS) : benzinS );
            naphtaS = (naphta < 0) ? red(naphtaS) : ((naphta > 0) ? green(naphtaS) : naphtaS );
            asphaltS = (asphalt < 0) ? red(asphaltS) : ((asphalt > 0) ? green(asphaltS) : asphaltS );
            
            std::cout << bold("Demand satisfaction:\n");
            std::cout << italic("\t- benzin\t") << benzinS << "\n";
            std::cout << italic("\t- naphta\t") << naphtaS << "\n";
            std::cout << italic("\t- asphalt\t") << asphaltS << "\n";
            std::cout << "\n";
            mproducts = Products();
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
        // outputs
        Products mproducts; /**< Current production. */
        
};

/** @}*/
/* ------------------------------------------------------------------------------------ */

#endif // SIMULATOR