#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "central.h"
#include "pipeline.h"
#include "rafinery.h"
#include "tools.h"

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
        Productor getProductor() { return [this](Products p){ this->AcquireProducts(p); }; }


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


#endif // SIMULATOR