
/**
 * @file simulator.cpp
 * @authors xbenes49 xpolan09
 * @date 27th november 2018
 * @brief Simulator class definition.
 *
 * This module implements Simulator class.
 */

#include "simulator.h"


struct ConsoleFirst EventOrder; /**< Definition of event order. */



Simulator::Simulator() {
    // first calendar event
    Activate(Time);

    // create pipelines
    Druzba = new OilPipeline("Druzba", 24.66, 10.55, 3);
    IKL = new OilPipeline("IKL", 27.4, 9.93, 2);
    // create rafineries
    Kralupy = new Rafinery("Kralupy", 9.04, 0);
    Litvinov = new Rafinery("Litvinov", 14.79, 0);
    // create Litvinov pipe
    Cent_Litvinov_Pipe = new Pipe("Centre_Litvinov", 20, 1, Litvinov->getInput());

    // send production of rafineries
    Kralupy->setProductor( getProductor() );
    Litvinov->setProductor( getProductor() );

    // create reserve
    CTR = new Reserve("Nelahozeves", 1293.5);

    // create central
    CentralaKralupy = new Central(Kralupy, Litvinov, Druzba, IKL, Cent_Litvinov_Pipe, CTR, demand, import);
    // connect pipelines to central
    Druzba->setOutput([this](double amount) {
        #ifdef CENTRAL_LOG
            std::cerr << Time << ") Central: Received " << amount << " from Druzba.\n";
        #endif
        this->CentralaKralupy->Enter(amount);
    });
    IKL->setOutput([this](double amount) {
        #ifdef CENTRAL_LOG
            std::cerr << Time << ") Central: Received " << amount << " from IKL.\n";
        #endif
        this->CentralaKralupy->Enter(amount);
    });
}


void Simulator::TerminalLoop() {
    std::string mem; /**< Memory for last command. */
    int skip = 0; /**< Skipping count */
    // day loop
    do {
        #ifdef SIMULATOR_LOG
            std::cerr << "Simulator step.\n";
        #endif
        bool newinput = false; /**< Indicates repeating of input. */
        bool invalid = false; /**< Indicates invalid input. */
        std::string line; /**< Input line */

        // input loop
        do {
            // skip, if set
            if(skip > 0) { skip--; break; }
            else { skipping = false; }
            // initialize indicators
            newinput = false;
            invalid = false;

            // print prompt
            std::cout << "[Day " << Time << "] >> " << std::flush;
            // get input
            getline(std::cin, line);
            // process input
            if(std::cin.eof()) { std::cout << bold("\nQuit.\n"); exit(0); }
            if(mem != "" && line == "") { line = mem; }
            std::vector<std::string> split = SplitString(line);

            // no content
            if(line == "" || split.size() == 0) {
                newinput = true;

            // next day
            } else if(split[0] == "next"
                   || split[0] == "n") {

            // current day
            } else if(split[0] == "day"
                   || split[0] == "now") {
                std::cout << "Now is " << style(std::string("Day ")+std::to_string( int(Time)), BOLD) << ".\n\n";
                newinput = true;

            // skip days
            } else if(split[0] == "skip"
                   || split[0] == "s") {
                if(split.size() == 2) {
                    skip = std::stoi(split[1]) - 1;
                    skipping = true;
                    std::cout << "Skipping " << skip+1 << " days.\n\n";
                } else {
                    invalid = true;
                }

            // demand
            } else if(split[0] == "demand"
                   || split[0] == "d") {
                newinput = true;
                // print all request values
                if(split.size() == 1) {
                    std::cout << bold("Current demand:\n");
                    std::cout << italic("- benzin: ") << demand.benzin << "\n";
                    std::cout << italic("- naphta: ") << demand.naphta << "\n";
                    std::cout << italic("- asphalt: ") << demand.asphalt << "\n";
                    const Demand& productionDemand = CentralaKralupy->getProductionDemand();
                    const Import& importOver = CentralaKralupy->getImportOver();
                    double oilNeed = max_3((productionDemand.benzin-importOver.benzin)/Fraction_Benzin, (productionDemand.naphta-importOver.naphta)/Fraction_Naphta, (productionDemand.asphalt-importOver.asphalt)/Fraction_Asphalt);
                    std::cout << cropTo0(oilNeed) << " of oil needed.\n";
                    int satisfy = (CTR->Level()/oilNeed);
                    std::string satisfyS = (satisfy < 90) ? red( double2str(satisfy) ) : green( double2str(satisfy) );
                    if(oilNeed < 0) satisfyS = green("eternity");
                    std::cout << "Current demand can be (at least partially) satisfied for " << satisfyS << " days.\n\n";
                // benzin
                } else if(split[1] == "benzin"
                       || split[1] == "natural"
                       || split[1] == "b") {
                    // print benzin request value
                    if(split.size() == 2) {
                        std::cout << italic("Benzin demand: ") << demand.benzin << "\n\n";
                    // set benzin request value
                    } else if(split.size() == 3) {
                        demand.benzin = std::stod(split[2]);
                        std::cerr << italic("New benzin value") << " is " << demand.benzin << "\n\n";
                    // error
                    } else {
                        invalid = true;
                    }
                // naphta
                } else if(split[1] == "naphta"
                       || split[1] == "nafta"
                       || split[1] == "n"
                       || split[1] == "diesel"
                       || split[1] == "d") {
                    // print naphta request value
                    if(split.size() == 2) {
                            std::cout << italic("Naphta demand: ") << demand.naphta << "\n\n";
                    // set naphta request value
                    } else if(split.size() == 3) {
                        demand.naphta = std::stod(split[2]);
                        std::cerr << italic("New naphta value") << " is " << demand.naphta << "\n\n";
                    // error
                    } else {
                        std::cerr << "Invalid input.\n";
                        invalid = true;
                    }
                // asphalt
                } else if(split[1] == "asphalt"
                       || split[1] == "asfalt"
                       || split[1] == "a") {
                    // print asphalt request value
                    if(split.size() == 2) {
                        std::cout << italic("Asphalt demand: ") << demand.asphalt << "\n\n";
                    // set asphalt request value
                    } else if(split.size() == 3) {
                        demand.asphalt = std::stod(split[2]);
                        std::cerr << italic("New asphalt value") << " is " << demand.asphalt << "\n\n";
                    // error
                    } else {
                        invalid = true;
                    }
                // error
                } else {
                    invalid = true;
                }
                CentralaKralupy->recountImport();

            // break / fix
            } else if(split[0] == "break"
                   || split[0] == "b"
                   || split[0] == "fix"
                   || split[0] == "f") {
                bool fix = (split[0] == "fix" || split[0] == "f");
                newinput = true;
                if(split.size() == 2) {
                    // druzba
                    if(split[1] == "druzba" || split[1] == "druzhba" || split[1] == "d") {
                        // fix druzba
                        if(fix) {
                            std::cout << bold("Druzba") << " pipeline fixed.\n";
                            Druzba->Fix();
                        // break druzba
                        } else {
                            std::cout << bold("Druzba") << " pipeline broken.\n";
                            Druzba->Break();
                        }
                        std::cout << "\n";
                    // IKL
                    } else if(split[1] == "ikl" || split[1] == "i") {
                        // fix IKL
                        if(fix) {
                            std::cout << bold("IKL") << " pipeline fixed.\n";
                            IKL->Fix();
                        // break IKL
                        } else {
                            std::cout << bold("IKL") << " pipeline broken.\n";
                            IKL->Break();
                        }
                        std::cout << "\n";
                    // Kralupy
                    } else if(split[1] == "kralupy" || split[1] == "k") {
                        // fix Kralupy
                        if(fix) {
                            std::cout << bold("Kralupy") << " rafinery fixed.\n";
                            Kralupy->Fix();
                        // break Kralupy
                        } else {
                            std::cout << bold("Kralupy") << " rafinery broken.\n";
                            Kralupy->Break();
                        }
                        std::cout << "\n";
                    // Litvinov
                    } else if(split[1] == "litvinov" || split[1] == "l") {
                        // fix Litvinov
                        if(fix) {
                            std::cout << bold("Litvinov") << " rafinery fixed.\n";
                            Litvinov->Fix();
                        // break Litvinov
                        } else {
                            std::cout << bold("Litvinov") << " rafinery broken.\n";
                            Litvinov->Break();
                        }
                        std::cout << "\n";
                    // all
                    } else if(split[1] == "all" || split[1] == "a") {
                        // fix all
                        if(fix) {
                            if(Druzba->IsBroken()) { std::cout << bold("Druzba") << " pipeline fixed.\n"; Druzba->Fix();}
                            if(IKL->IsBroken()) { std::cout << bold("IKL") << " pipeline fixed.\n"; IKL->Fix(); }
                            if(Kralupy->IsBroken()) { std::cout << bold("Kralupy") << " rafinery fixed.\n"; Kralupy->Fix(); }
                            if(Litvinov->IsBroken()) { std::cout << bold("Litvinov") << " rafinery fixed.\n"; Litvinov->Fix(); }
                        // break all
                        } else {
                            if(!Druzba->IsBroken()) { std::cout << bold("Druzba") << " pipeline broken.\n"; Druzba->Break(); }
                            if(!IKL->IsBroken()) { std::cout << bold("IKL") << " pipeline broken.\n"; IKL->Break(); }
                            if(!Kralupy->IsBroken()) { std::cout << bold("Kralupy") << " rafinery broken.\n"; Kralupy->Break(); }
                            if(!Litvinov->IsBroken()) { std::cout << bold("Litvinov") << " rafinery broken.\n"; Litvinov->Break(); }
                        }
                        std::cout << "\n";
                    // error
                    } else {
                        invalid = true;
                    }
                // error
                } else {
                    invalid = true;
                }

            // help
            } else if(split[0] == "help" || split[0] == "h") {
                newinput = true;
                std::cout << bold("Console commands: \n");
                std::cout << italic("Next") << "                          Count next day.\n";
                std::cout << italic("Import") << "                        Print current import.\n";
                std::cout << italic("Import <comodity>") << "             Print current import of comodity.\n";
                std::cout << italic("Import <comodity> <number>") << "    Set current import of comodity.\n";
                std::cout << italic("Demand") << "                        Print current demand.\n";
                std::cout << italic("Demand <comodity>") << "             Print current demand of comodity.\n";
                std::cout << italic("Demand <comodity> <number>") << "    Set current demand of comodity.\n";
                std::cout << italic("Break <facility>") << "              Breaks facility.\n";
                std::cout << italic("Fix <facility>") << "                Fixes facility.\n";
                std::cout << italic("Status") << "                        Prints status of system at current day.\n";
                std::cout << italic("Status <facility>") << "             Prints status of facility at current day.\n";
                std::cout << italic("Day") << "                           Shows current day.\n";
                std::cout << italic("Skip <number>") << "                 Skip number of days.\n";
                std::cout << italic("Help") << "                          Prints this help.\n";
                std::cout << bold("\nComodities:\n");
                std::cout << italic("\tbenzin") << "|natural|b\n";
                std::cout << italic("\tnaphta") << "|nafta|diesel|n|d\n";
                std::cout << italic("\tasphalt") << "|asfalt|a\n";
                std::cout << bold("\nFacilities:\n");
                std::cout << italic("\tDruzba") << "|Druzhba|d\n";
                std::cout << italic("\tIKL") << "|i\n";
                std::cout << italic("\tKralupy") << "|k\n";
                std::cout << italic("\tLitvinov") << "|l\n";

            // status
            } else if(split[0] == "status" || split[0] == "stat") {
                newinput = true;
                // get statuses
                PipelineStatus druzbastat = Druzba->getStatus();
                PipelineStatus iklstat = IKL->getStatus();
                RafineryStatus kralupystat = Kralupy->getStatus();
                RafineryStatus litvinovstat = Litvinov->getStatus();
                ReserveStatus ctrstat = CTR->getStatus();
                if(split.size() > 1) {
                    // status druzba
                    if(split[1] == "druzba" || split[1] == "druzhba" || split[1] == "d") {
                        druzbastat.print();
                        for(auto& it: druzbastat.delivery) {
                            if(it.second > 0.001) {
                                std::cout << "Time " << it.first << ": " << it.second << "\n";
                            }
                        }
                        std::cout << "\n";
                    // status ikl
                    } else if(split[1] == "ikl" || split[1] == "i") {
                        iklstat.print();
                        for(auto& it: iklstat.delivery) {
                            if(it.second > 0.001) {
                                std::cout << "Time " << it.first << ": " << it.second << "\n";
                            }
                        }
                        std::cout << "\n";
                    // status kralupy
                    } else if(split[1] == "kralupy" || split[1] == "k") {
                        kralupystat.print();
                        for(auto& it: kralupystat.production) {
                            if(it.second > 0.001) {
                                std::cout << "Time " << it.first << ": " << it.second << "\n";
                            }
                        }
                        std::cout << "\n";
                    // status litvinov
                    } else if(split[1] == "litvinov" || split[1] == "l") {
                        litvinovstat.print();
                        for(auto& it: litvinovstat.production) {
                            if(it.second > 0.001) {
                                std::cout << "Time " << it.first << ": " << it.second << "\n";
                            }
                        }
                        std::cout << "\n";
                    // status nelahozeves
                    } else if(split[1] == "nelahozeves" || split[1] == "ctr") {
                        ctrstat.print();
                        if(ctrstat.requested != -1) {
                            std::cout << "Taken " << red( double2str(ctrstat.given) ) << " [" << italic( double2str(ctrstat.requested) ) << " requested]\n";
                        }
                        if(ctrstat.added != -1) {
                            std::cout << "Added " << green( double2str(ctrstat.added) ) << " [" << italic( double2str(ctrstat.returned) ) << " returned]\n";
                        }
                        std::cout << "\n";

                    // error
                    } else {
                        invalid = true;
                    }
                // status
                } else {
                    druzbastat.print();
                    iklstat.print();
                    kralupystat.print();
                    litvinovstat.print();
                    ctrstat.print();
                    std::cout << "\n";
                }

            // import
            } else if(split[0] == "import" || split[0] == "i") {
                newinput = true;
                if(split.size() == 1) {
                    std::cout << bold("Current import:\n");
                    std::cout << italic("- benzin: ") << import.benzin << "\n";
                    std::cout << italic("- naphta: ") << import.naphta << "\n";
                    std::cout << italic("- asphalt: ") << import.asphalt << "\n\n";
                // benzin
                } else if(split[1] == "benzin"
                       || split[1] == "natural"
                       || split[1] == "b") {
                    // print benzin request value
                    if(split.size() == 2) {
                        std::cout << italic("Benzin import: ") << import.benzin << "\n\n";
                    // set benzin import value
                    } else if(split.size() == 3) {
                        import.benzin = std::stod(split[2]);
                        std::cerr << italic("New benzin import value") << " is " << import.benzin << "\n\n";
                    // error
                    } else {
                        invalid = true;
                    }
                // naphta
                } else if(split[1] == "naphta"
                       || split[1] == "nafta"
                       || split[1] == "n"
                       || split[1] == "diesel"
                       || split[1] == "d") {
                    // print naphta import value
                    if(split.size() == 2) {
                            std::cout << italic("Naphta import: ") << import.naphta << "\n\n";
                    // set naphta import value
                    } else if(split.size() == 3) {
                        import.naphta = std::stod(split[2]);
                        std::cerr << italic("New naphta import value") << " is " << import.naphta << "\n\n";
                    // error
                    } else {
                        std::cerr << "Invalid input.\n";
                        invalid = true;
                    }
                // asphalt
                } else if(split[1] == "asphalt"
                       || split[1] == "asfalt"
                       || split[1] == "a") {
                    // print asphalt import value
                    if(split.size() == 2) {
                        std::cout << italic("Asphalt import: ") << import.asphalt << "\n\n";
                    // set asphalt request value
                    } else if(split.size() == 3) {
                        import.asphalt = std::stod(split[2]);
                        std::cerr << italic("New asphalt value") << " is " << import.asphalt << "\n\n";
                    // error
                    } else {
                        invalid = true;
                    }
                // error
                } else {
                    invalid = true;
                }
                CentralaKralupy->recountImport();

            // exit
            } else if(split[0] == "quit" || split[0] == "exit" || split[0] == "q") {
                std::cout << bold("Quit.\n");
                exit(0);

            // unknown command
            } else {
                invalid = true;
            }

            // command to memory
            if(!invalid) mem = line;
            // invalid
            else { std::cerr << red("Invalid input.\n"); }

        // loop command input
        } while(newinput || invalid);

        // make command line first in day
        EventOrder.t = Time+1;
        Seize(EventOrder.waitForConsole);
        Wait(1);
        ResolveDayDemand();
        Release(EventOrder.waitForConsole);

    // loop days
    } while(true);
}
