
#include <algorithm>
#include <cstring>
#include <iostream>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "simlib.h"

typedef std::function<void(double)> Callback;

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
    //#define RESERVE_LOG
#endif

struct ConsoleFirst {
    Facility waitForConsole;
    double t = Time;
} EventOrder;

struct Products {
	double benzin = 0;
	double naphta = 0;
	double asphalt = 0;

    Products& operator+=(const Products& other) {
        this->benzin += other.benzin;
        this->naphta += other.naphta;
        this->asphalt += other.asphalt;
        return *this;
    }
};
typedef std::function<void(Products)> Productor;

class Source : public Process {
    public:
        Source(std::string name, double production, Callback output):
            mname(name), mproduction(production), moutput(output) {}
        void Behavior() {
            do {

                bool seize = (Time >= EventOrder.t);
                if(seize) Seize(EventOrder.waitForConsole);

                #ifdef SOURCE_LOG
                    std::cerr << Time << ") Source " << mname << ": Produced " << mproduction << ".\n";
                #endif
                moutput(mproduction);

                if(seize) Release(EventOrder.waitForConsole);
                Wait(1);   
            } while(true);

            
        }
    private:
        std::string mname;
        double mproduction;
        Callback moutput;

        long id;
        long& getCnt() { static long i = 1; return i; }
};

class Flagger {
    public:
        Flagger() {}
        double Check(double amount) {
            if(mbroken) return 0;
            else return amount;
        }
        void Set() { mbroken = true; }
        void Reset() { mbroken = false; }
        void Set(bool v) { mbroken = !v; }
        bool IsSet() { return mbroken; }
    private:
        bool mbroken = false;
};

class InputLimiter {
    public:
        InputLimiter(double maximum):
            mmaximum(maximum) {}
        double output(double amount, double basis=0) { return (amount < (mmaximum-basis))?amount:(mmaximum-basis); }
        double rest(double amount, double basis=0) { return (amount < (mmaximum-basis))?0:(amount-mmaximum+basis); }
    private:
        double mmaximum;
};

class Transfer: public Process {
    public:
        Transfer(double amount, Callback output):
            mamount(amount), moutput(output) {
            #ifdef TRANSFER_LOG
                std::cerr << Time << ") Transfer of " << mamount << " initialized.\n";
            #endif
        }

        void Behavior() {
            bool seize = (Time >= EventOrder.t);
            if(seize) {
                Seize(EventOrder.waitForConsole);
            }
            #ifdef TRANSFER_LOG
                std::cerr << Time << ") Transfer: transferred " << mamount << ".\n";
            #endif
            moutput(mamount);
            if(seize) {
                Release(EventOrder.waitForConsole);
            }
        }


    private:
        double mamount;
        Callback moutput;

};
class Pipe {
    public:
        Pipe(std::string name, double maximum, double delay, Callback output=[](double){ std::cerr << "Output not set!\n"; }):
            mname(name), il(maximum), d(delay), moutput(output) {
        }
        void Send(double amount) {
            amount = f.Check(amount);
            PlanSending(amount, Time);

            if(sending.count(Time + d) != 0) {
                #ifdef PIPES_LOG
                    std::cerr << Time << ") Pipe " << mname << ": Sending " << sending[Time+d] << ".\n";
                #endif
                (new Transfer(sending[Time+d], moutput))->Activate(Time + d);
            }
            sending.erase(Time+d);
        }
        Callback getInput() { return [this](double amount){ this->Send(amount);}; }
        void setOutput(Callback output) { moutput = output; }

        void Break() { f.Set(); }
        void Fix() { f.Reset(); }
        bool IsBroken() { return f.IsSet(); }

    private:
        std::string mname;
        InputLimiter il;
        double d;
        Callback moutput;

        double maxStorage = 100;
        std::map<double, double> sending;
        void PlanSending(double amount, double t) {
            if(amount == 0) return;
            double sum;
            // sum all in <time, time+d>
            sum = 0;
            for(int i = t; i <= (t+d); i++) {
                if(sending.count(i)) sum += sending[i];
            }
            if((sum+amount) > maxStorage) {
                amount = maxStorage - sum;
            }
            sending[t+d] += il.output(amount, sum);
            PlanSending(il.rest(amount, sum), t+1);
        }
        Flagger f;
};

class Reserve {
    public:
        Reserve(std::string name, double capacity, double maxTransport, double delay, Callback receiveFunc):
            mname(name), il(capacity), mlevel(capacity) {
            there = new Pipe(mname+"_there", maxTransport, delay, getInput());
            back = new Pipe(mname+"_back", maxTransport, delay, receiveFunc);
        }

        void Send(double amount) {
            there->Send(amount);
        }
        Callback getInput() { return [this](double amount){ this->Received(amount); }; }

        void Received(double amount) {
            (void)amount;
            #ifdef RESERVE_LOG
                std::cerr << Time << ") Reserve " << mname << ": Received " << amount << ".\n";
            #endif
            //back->Send(<neco>); // odesle, co se nevejde
        }

        void Request(double amount) {
            // limit by the limit of reserves
            back->Send(amount);
        }

        double Missing() {
            return 0;
            // how much is missing (to full/to accomplish the EU rules?)
        }

        double Level() { return mlevel; }

    private:
        std::string mname;
        InputLimiter il;
        double mlevel;

        Pipe *there;
        Pipe *back;
};

class OilPipeline {
    public:
        OilPipeline(std::string name, double maxProduction, double producing, double delay):
            mname(name), mmaximum(maxProduction), mproducing(producing), mdelay(delay) {

            p = new Pipe(mname, mmaximum, mdelay, getOutput());
            s = new Source(mname, producing, p->getInput());
            s->Activate();
        }
        void Foo(double amount) {
            #ifdef PIPELINE_LOG
                std::cerr << Time << ") OilPipeline " << mname << ": Received " << amount << "\n";
            #endif
            moutput(amount);
        }
        Callback getOutput() { return [this](double amount){ this->Foo(amount); }; }


        bool IsBroken() { return p->IsBroken(); }   // returns true if the refinery is broken
        void Break() { p->Break(); }
        void Fix() { p->Fix(); }

        void setOutput(Callback output) { moutput = output; }

    private:
        std::string mname;
        double mmaximum;
        double mproducing;
        double mdelay;
        bool broken = false;

        Callback moutput;

        Pipe* p;
        Source* s;
};

class FractionalDestillation: public Event {
    public:
        FractionalDestillation(double amount, Productor output):
            mamount(amount), moutput(output) {}
        void Behavior() {
            Products p;
            p.benzin = 0.19*mamount;
            p.naphta = 0.42*mamount;
            p.asphalt = 0.13*mamount;
            moutput(p);
        }

    private:
        double mamount;
        Productor moutput;
};
class Rafinery: public Event {
    public:
        Rafinery(std::string name, double maxProcessing, double delay):
            mname(name), il(maxProcessing), d(delay) {}
        void Enter(double amount) {
            amount = f.Check(amount);
            PlanProcessing(amount, Time);

            if(processing.count(Time) > 0) {
                #ifdef RAFINERY_LOG
                    std::cerr << Time << ") Rafinery " << mname << ": Process " << amount << ".\n";
                #endif
                (new FractionalDestillation(processing[Time], getOutput()))->Activate(Time+d);
            }
        }
        Callback getInput() { return [this](double amount){this->Enter(amount);}; }

        void Behavior() {
            //moutput(processing.at(Time));
            #ifdef RAFINERY_LOG
                std::cerr << Time << ") Rafinery " << mname << ": Processed " << processing.at(Time) << "\n";
            #endif
            processing.erase(Time);
        }

        bool IsBroken() { return f.IsSet(); }
        void Break() { f.Set(); }
        void Fix() { f.Reset(); }

        void output(Products p) {
            #ifdef RAFINERY_LOG
                std::cerr << Time << ") Rafinery " << mname << ": Processed ["<<"b:"<<p.benzin<<", "
                                                                              <<"n:"<<p.naphta<<", "
                                                                              <<"a:"<<p.asphalt<<"].\n";
            #endif
            mproductor(p);
        }
        Productor getOutput() { return [this](Products p){this->output(p);}; }
        void setProductor(Productor p) {
            mproductor = p;
        }

    private:
        std::string mname;
        InputLimiter il;
        double d;
        Flagger f;

        double maxStorage = 100;
        std::map<double, double> processing;
        void PlanProcessing(double amount, double t) {
            if(amount == 0) return;
            double sum;
            // sum all in <time, time+d>
            sum = 0;
            for(int i = t; i <= (t+d); i++) {
                if(processing.count(i)) sum += processing[i];
            }
            if((sum+amount) > maxStorage) {
                amount = maxStorage - sum;
            }
            processing[t+d] += il.output(amount, sum);
            PlanProcessing(il.rest(amount, sum), t+1);
        }
        Productor mproductor;
};
// constants from documentation
const double IKL_Ratio = 0.484863281;
const double Druzba_Ratio = 0.515136718;
const double Kralupy_Ratio = 0.379353755;
const double Litvinov_Ratio = 0.620646244;

const double Fraction_Benzin = 0.19;
const double Fraction_Naphta = 0.42;
const double Fraction_Asphalt = 0.13;

struct CentInputRatio{
    double IKL = IKL_Ratio;
    double Druzba = Druzba_Ratio;
};
struct CentOutputRatio{
    double Kralupy = Kralupy_Ratio;
    double Litvinov = Litvinov_Ratio;
};
struct Demand{
    double benzin = 4.38;
    double naphta = 12.96;
    double asphalt = 1.21;
};

class Central {
    public:
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

            // demand correction - DEMAND FIRST, RESERVE SECOND
            double demandOil = max_3(demand.benzin/Fraction_Benzin, demand.naphta/Fraction_Naphta, demand.asphalt/Fraction_Asphalt);
            if(demandOil > amount) {
                // ask reserve for oil
                // CTR->Request(demandOil-amount);
                // potrebuju se od rezervy dozvedet, kolik ropy jsem dostal
                // + je opravdu potreba to posilat trubkou s delay 0?
            }
            else {
                // reserve interactions
                double missing = CTR->Missing();
                double canSend = amount - demandOil;
                if(missing && canSend != 0.0) {
                    #ifdef DISTRIBUTE_LOG
                        std::cerr << Time << ") Central: Sending " << ((missing<=amount)?missing:amount) << " to CTR\n";
                    #endif
                    CTR->getInput()((missing<=canSend)?missing:canSend);
                    amount = (missing<=canSend)?amount-missing:amount-canSend;
                }
            }
            // TODO: reflektovat potrebu ropy - vyslat pozadavek na ropovody (missing,demand,amount,input)


            #ifdef DISTRIBUTE_LOG
                std::cerr << Time << ") Central: Sending " << amount*output.Kralupy << " to Kralupy\n";
            #endif
            koutput(amount*output.Kralupy);
            #ifdef DISTRIBUTE_LOG
                std::cerr << Time << ") Central: Sending " << amount*output.Litvinov << " to Litvinov\n";
            #endif
            loutput(amount*output.Litvinov);
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
        Pipe* LitPipe;                      // oil for Litvinov is sent via pipe
        Reserve* CTR;
        struct Demand& demand;
};

class Simulator: public Process {
    public:
        Simulator() {
            Activate(Time);

            Druzba = new OilPipeline("Druzba", 24.66, 10.55, 3);
            IKL = new OilPipeline("IKL", 27.4, 9.93, 2);
            Kralupy = new Rafinery("Kralupy", 9.04, 1);
            Litvinov = new Rafinery("Litvinov", 14.79, 1);
            Cent_Litvinov_Pipe = new Pipe("Centre_Litvinov", 10/*change*/, 1, Litvinov->getInput());

            Kralupy->setProductor( getProductor() );
            Litvinov->setProductor( getProductor() );

            CTR = new Reserve("Nelahozeves", 1293.5, 50, 0,
                /* sem se zapoji Central*/[](double amount){ std::cerr << Time << ") ControlCenter: Receive " << amount << ".\n";}
            );

            CentralaKralupy = new Central(Kralupy, Litvinov, Druzba, IKL, Cent_Litvinov_Pipe, CTR, demand);
            Callback CentralInputFromDruzba = [this](double amount) {
                #ifdef CENTRAL_LOG
                    std::cerr << Time << ") Central: Received " << amount << " from Druzba.\n";
                #endif
                this->CentralaKralupy->Enter(amount);
            };
            Callback CentralInputFromIKL = [this](double amount) {
                #ifdef CENTRAL_LOG
                    std::cerr << Time << ") Central: Received " << amount << " from IKL.\n";
                #endif
                this->CentralaKralupy->Enter(amount);
            };
            Druzba->setOutput( CentralInputFromDruzba );
            IKL->setOutput( CentralInputFromIKL );
        }

        void AcquireProducts(Products p) {
            mproducts += p;
        }
        Productor getProductor() { return [this](Products p){ this->AcquireProducts(p); }; }

        std::vector<std::string> SplitString(std::string str) {
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

        void Behavior() {
            std::string mem;
            do {
                #ifdef SIMULATOR_LOG
                std::cerr << "Simulator step.\n";
                #endif
                bool newinput = false;
                bool invalid = false;
                std::string line;
                do {
                    newinput = false;
                    invalid = false;
                    std::cout << ">> " << std::flush;
                    getline(std::cin, line);
                    if(std::cin.eof()) { std::cout << "Quit\n"; exit(0); }
                    if(mem != "" && line == "") { line = mem; }
                    std::vector<std::string> split = SplitString(line);

                    // no content
                    if(line == "" || split.size() == 0) {
                        newinput = true;

                    // next day
                    } else if(split[0] == "next"
                      || split[0] == "n") {            
                        std::cerr << "Day "<<Time<<".\n";

                    // change request
                    } else if(split[0] == "demand"
                           || split[0] == "d") {
                        invalid = true;
                        if(split.size() <= 1) { // print all request values
                            std::cout << "Current demand:\n";
                            std::cout << "- benzin: " << demand.benzin << "\n";
                            std::cout << "- naphta: " << demand.naphta << "\n";
                            std::cout << "- asphalt: " << demand.asphalt << "\n";
                        // benzin    
                        } else if(split[1] == "benzin" 
                               || split[1] == "natural"
                               || split[1] == "b") {
                            if(split.size() == 2) {         // print benzin request value
                                std::cout << "Benzin demand: " << demand.benzin << "\n";
                            } else if(split.size() == 3) {  // set benzin request value
                                double val = std::stod(split[2]);
                                std::cerr << "New benzin value is " << val << "\n";
                            } else {                        // error
                                std::cerr << "Invalid input.\n";
                                invalid = true;
                            }

                        // naphta
                        } else if(split[1] == "naphta" || split[1] == "nafta" || split[1] == "n" 
                               || split[1] == "diesel" || split[1] == "d") {
                            if(split.size() == 2) {         // print naphta request value
                                std::cerr << "Maphta demand:\n";
                                std::cout << "Naphta demand: " << demand.naphta << "\n";
                            } else if(split.size() == 3) {  // set naphta request value
                                double val = std::stod(split[2]);
                                std::cerr << "New naphta value is " << val << "\n";
                            } else {                        // error
                                std::cerr << "Invalid input.\n";
                                invalid = true;
                            }
                        
                        // asphalt
                        } else if(split[1] == "asphalt" || split[1] == "asfalt" || split[1] == "a") {
                            if(split.size() == 2) {         // print asphalt request value
                                std::cout << "Asphalt demand: " << demand.asphalt << "\n";
                            } else if(split.size() == 3) {  // set asphalt request value
                                double val = std::stod(split[2]);
                                std::cerr << "New asphalt value is " << val << "\n";
                            } else {                        // error
                                std::cerr << "Invalid input.\n";
                                
                            }
                        
                        // error
                        } else {
                            std::cerr << "Invalid input.\n";
                            invalid = true;
                        }
                    
                    // break
                    } else if(split[0] == "break" || split[0] == "b" || split[0] == "fix" || split[0] == "f") {
                        bool fix = (split[0] == "fix" || split[0] == "f");
                        newinput = true;
                        if(split.size() == 2) {
                            if(split[1] == "druzba" || split[1] == "druzhba" || split[1] == "d") {
                                if(fix) { Druzba->Fix(); std::cerr << "Fix "; }
                                else { Druzba->Break(); std::cerr << "Break "; }
                                std::cerr << "Druzba.\n";
                            } else if(split[1] == "ikl" || split[1] == "i") {
                                if(fix) { IKL->Fix(); std::cerr << "Fix "; }
                                else { IKL->Break(); std::cerr << "Break "; }
                                std::cerr << "IKL.\n";
                            } else if(split[1] == "kralupy" || split[1] == "k") {
                                if(fix) { Kralupy->Fix(); std::cerr << "Fix "; }
                                else { Kralupy->Break(); std::cerr << "Break "; }
                                std::cerr << "Kralupy.\n";
                            } else if(split[1] == "litvinov" || split[1] == "l") {
                                if(fix) { Litvinov->Fix(); std::cerr << "Fix "; }
                                else { Litvinov->Break(); std::cerr << "Break "; }
                                std::cerr << "Litvinov.\n";
                            } else {
                                std::cerr << "Invalid input.\n";
                                invalid = true;
                            }
                        }

                    // exit
                    } else if(split[0] == "quit" || split[0] == "exit" || split[0] == "q") {
                        exit(0);

                    // unknown command
                    } else {
                        std::cerr << "Invalid input.\n";
                        invalid = true;
                    }
                    if(!invalid) mem = line;
                } while(newinput || invalid);
                
                EventOrder.t = Time+1;
                Seize(EventOrder.waitForConsole);
                Wait(1);
                Release(EventOrder.waitForConsole);

            } while(true);
        }

    private:
        OilPipeline* Druzba;
        OilPipeline* IKL;
        Rafinery* Kralupy;
        Rafinery* Litvinov;
        Reserve* CTR;
        Central* CentralaKralupy;

        Products mproducts;
        Pipe* Cent_Litvinov_Pipe;
        Demand demand;
};

int main() {
    Print("Model Ropovod - SIMLIB/C++\n");
    Init(1,100);
    new Simulator();
    Run();
    return 0;
}
