
#include <iostream>
#include <functional>
#include <map>
#include <string>

#include "simlib.h"

typedef std::function<void(double)> Callback;

#ifdef OUTPUT_LOG
    #define PIPES_LOG
    //#define SOURCE_LOG
    //#define FLAGGER_LOG
    //#define SIMULATOR_LOG
    //#define RAFINERY_LOG
    //#define PIPELINE_LOG
    #define DISTRIBUTE_LOG
    #define CENTRAL_LOG
    #define TRANSFER_LOG
#endif

class Source : public Event {
    public:
        Source(std::string name, double production, Callback output):
            mname(name), mproduction(production), moutput(output) {}
        void Behavior() {
            #ifdef SOURCE_LOG
                std::cerr << Time << ") Source " << mname << ": Produced " << mproduction << ".\n";
            #endif
            moutput(mproduction);
            Activate(Time+1);
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
    private:
        bool mbroken = false;
};

class InputLimiter {
    public:
        InputLimiter(double maximum):
            mmaximum(maximum) {}
        double output(double amount) { return (amount < mmaximum)?amount:mmaximum; }
        double rest(double amount) { return (amount < mmaximum)?0:(amount-mmaximum); }
    private:
        double mmaximum;
};

class Transfer: public Event {
    public:
        Transfer(double amount, Callback output):
            mamount(amount), moutput(output) {}

        void Behavior() {
            #ifdef TRANSFER_LOG
                std::cerr << Time << ") Transfer: transferred " << mamount << ".\n";
            #endif
            moutput(mamount);
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
            #ifdef PIPES_LOG
                std::cerr << Time << ") Pipe " << mname << ": Sending " << amount << ".\n";
            #endif

            if(sending.count(Time + d) == 0) {
                (new Transfer(amount, moutput))->Activate(Time + d);
            }
            sending[Time + d] += amount;
            // udelat rozpocitavac na jednotlive casy (Input Limiter)
        }
        Callback getInput() { return [this](double amount){ this->Send(amount);}; }
        void setOutput(Callback output) { moutput = output; }

        void SetBroken() { f.Set(); }
        void SetWorking() { f.Reset(); }

    private:
        std::string mname;
        InputLimiter il;
        double d;
        Callback moutput;

        int i=0;

        std::map<double, double> sending;
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
            std::cerr << Time << ") Reserve " << mname << ": Received " << amount << ".\n";
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

        void setOutput(Callback output)
        {
            moutput = output;
            //p->setOutput(output);
        }

    private:
        std::string mname;
        double mmaximum;
        double mproducing;
        double mdelay;

        Callback moutput;

        Pipe* p;
        Source* s;
};

class Rafinery: public Event {
    public:
        Rafinery(std::string name, double maxProcessing, double delay):
            mname(name), il(maxProcessing), d(delay) {}
        void Enter(double amount) {
            if(processing.count(Time + d) == 0) Activate(Time + d);
            processing[Time + d] += amount;
        }
        Callback getInput() { return [this](double amount){this->Enter(amount);}; }

        void Behavior() {
            //moutput(processing.at(Time));
            #ifdef RAFINERY_LOG
                std::cerr << Time << ") Rafinery " << mname << ": Processed " << processing.at(Time) << "\n";
            #endif
            processing.erase(Time);
        }

    private:
        std::string mname;
        InputLimiter il;
        double d;

        std::map<double, double> processing;
};

struct CentInputRatio{
    double IKL = 0.484863281;
    double Druzhba = 0.515136718;
};
struct CentOutputRatio{
    double Kralupy = 0.379353755;
    double Litvinov = 0.620646244;
};

class Central {
    public:
        Central(Rafinery *kr, Rafinery *lit, Pipe* Cent_Litvinov_Pipe):
            Kralupy(kr), Litvinov(lit), LitPipe(Cent_Litvinov_Pipe)
            {
                koutput = Kralupy->getInput();
                loutput = LitPipe->getInput();
            }
        void Enter(double amount) {
            #ifdef DISTRIBUTE_LOG
                std::cerr << ")\t\t" << "Central: Sending " << amount*output.Kralupy << " to Kralupy\n";
            #endif
            koutput(amount*output.Kralupy);
            #ifdef DISTRIBUTE_LOG
                std::cerr << ")\t\t" << "Central: Sending " << amount*output.Litvinov << " to Litvinov\n";
            #endif
            loutput(amount*output.Litvinov);
        }
        Callback getInput() { return [this](double amount){this->Enter(amount);}; }

        void Behavior() {}
    private:
        struct CentInputRatio input;        // current ratio of input - negotiate with OilPipelines
        struct CentOutputRatio output;      // current ratio of output - negotiate with Rafinery
        Callback koutput;                   // output of central - input function of Kralupy
        Callback loutput;                   // output of central - input function of Litvinov
        Rafinery *Kralupy;
        Rafinery *Litvinov;
        Pipe* LitPipe;                      // oil for Litvinov is sent via pipe
};

struct Products {
	double benzin = 0;
	double naphta = 0;
	double asphalt = 0;
};

class Simulator: public Process {
    public:
        Simulator() {
            Druzba = new OilPipeline("Druzba", 24.66, 10.55, 3);
            IKL = new OilPipeline("IKL", 27.4, 9.93, 2);
            Kralupy = new Rafinery("Kralupy", 9.04, 1);
            Litvinov = new Rafinery("Litvinov", 14.79, 1);
            Cent_Litvinov_Pipe = new Pipe("Cent->Litvinov", 10/*change*/, 1, Litvinov->getInput());

            //Druzba->setOutput( Kralupy->getInput() );
            //IKL->setOutput( Kralupy->getInput() );

            CTR = new Reserve("Nelahozeves", 1293.5, 50, 0,
                /* sem se zapoji Central*/[](double amount){ std::cerr << Time << ") ControlCenter: Receive " << amount << ".\n";}
            );

            CentralaKralupy = new Central(Kralupy, Litvinov, Cent_Litvinov_Pipe);
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


            Activate();
        }

        void Behavior() {
            do {
                #ifdef SIMULATOR_LOG
                std::cerr << "Simulator step.\n";
                #endif
                //CTR->Send(1);
                //CTR->Request(1);
                Wait(1);
            } while(true);
        }

    private:
        OilPipeline* Druzba;
        OilPipeline* IKL;
        Rafinery* Kralupy;
        Rafinery* Litvinov;
        Reserve* CTR;
        Central* CentralaKralupy;
        Pipe* Cent_Litvinov_Pipe;
};

int main() {
    Print("Model Ropovod - SIMLIB/C++\n");
    Init(0,10);
    new Simulator();
    Run();
    return 0;
}
