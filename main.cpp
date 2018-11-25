
#include <iostream>
#include <functional>
#include <map>
#include <string>

#include "simlib.h"

typedef std::function<void(double)> Callback;

class Source : public Event {
    public:
        Source(double production, Callback output):
            mproduction(production), moutput(output) {}
        ~Source() { std::cerr << Time << ") Source: Destroy itself.\n"; }
        void Behavior() {
            std::cerr << Time << ") Source: Produced " << mproduction << ".\n";
            moutput(mproduction);
            std::cerr << Time << ") Source: Reactivate at " << Time+1 << ".\n";
            Activate(Time+1);
        }
    private:
        double mproduction;
        Callback moutput;
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
        InputLimiter(double maximum): mmaximum(maximum) {}
        double output(double amount) { return (amount < mmaximum)?amount:mmaximum; }
        double rest(double amount) { return (amount < mmaximum)?0:(amount-mmaximum); }
    private:
        double mmaximum;
};

class Pipe: public Event {
    public:
        Pipe(double delay, double maximum, Callback output):
            d(delay), moutput(output), il(maximum) {
        }
        ~Pipe() { std::cerr << Time << ") Pipe: Destroy itself.\n"; }
        void Send(double amount) {
            std::cerr << Time << ") Pipe: Sending " << amount << ".\n";
            if(sending.count(Time + d) == 0) Activate(Time + d);
            sending[Time + d] += amount;
            // udelat rozpocitavac na jednotlive casy (Input Limiter)
        }
        Callback getInput() { return [this](double amount){ this->Send(amount);}; }
        void Behavior() {
            moutput(sending[Time]);
            sending.erase(Time);
        }
        void SetBroken() { f.Set(); }
        void SetWorking() { f.Reset(); }

    private:
        double d;
        Callback moutput;
        std::map<double, double> sending;
        InputLimiter il;
        Flagger f;
};

class Reserve {
    public:
        Reserve(double capacity, double delay, double maximum, Callback receiveFunc):
            il(capacity), mlevel(capacity) {
            there = new Pipe(delay, maximum, getInput());
            back = new Pipe(delay, maximum, receiveFunc);
        }

        void Send(double amount) {
            there->Send(amount);
        }
        Callback getInput() { return [this](double amount){ this->Received(amount); }; }

        void Received(double amount) {
            std::cerr << Time << ") CTR: Received " << amount << ".\n";
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
        InputLimiter il;
        double mlevel;
        double mdelay;

        Pipe *there;
        Pipe *back;
};

class OilPipeline {
    public:
        OilPipeline(double delay, double maxProduction, double producing):
            mdelay(delay), mmaximum(maxProduction), mproducing(producing) {
            
            p = new Pipe(mdelay, mmaximum, getOutput());
            s = new Source(producing, p->getInput());
            s->Activate();
        }
        ~OilPipeline() { std::cerr << Time << ") OilPipeling: Destroy itself.\n"; }
        void Foo(double amount) {
            std::cerr << Time << ") OilPipeline: Received " << amount << "\n";
            moutput(amount);
        }
        Callback getOutput() { return [this](double amount){ this->Foo(amount); }; }

        void setOutput(Callback output) { moutput = output; }

        void Behavior() { s->Activate(Time); }
        
    private:
        double mdelay;
        double mmaximum;
        double mproducing;
        Callback moutput;

        Pipe* p;
        Source* s;
};

class Rafinery: public Process {
    public:
        Rafinery(double delay, double maxProcessing):
            d(delay), il(maxProcessing) {}
        ~Rafinery() { std::cerr << Time << ") Rafinery: Destroy itself.\n"; }
        void Enter(double amount) {
            if(processing.count(Time + d) == 0) Activate(Time + d);
            processing[Time + d] += amount;
        }
        Callback getInput() { return [this](double amount){this->Enter(amount);}; }

        void Behavior() {
            //moutput(processing.at(Time));
            std::cerr << Time << ") Rafinery: Processed " << processing.at(Time) << "\n";
            processing.erase(Time);
        }

    private:
        double d;
        InputLimiter il;
        std::map<double, double> processing;
};

class Simulator: public Process {
    public:
        Simulator() {
            Druzba = new OilPipeline(2, 200, 100);
            IKL = new OilPipeline(2, 200, 100);
            Kralupy = new Rafinery(5, 200);
            Litvinov = new Rafinery(5, 200);

            Druzba->setOutput( Kralupy->getInput() );
            IKL->setOutput( Kralupy->getInput() );

            CTR = new Reserve(1293.5, 0.1, 50, [](double){ std::cerr << Time << ") Receive to the control center!\n";} );

            Activate();
        }

        void Behavior() {
            do {
                std::cerr << "Simulator step.\n";
                //CTR->Send(1);
                CTR->Request(1);
                Wait(1);
            } while(true);
        }
    private:
        OilPipeline* Druzba;
        OilPipeline* IKL;
        Rafinery* Kralupy;
        Rafinery* Litvinov;
        Reserve* CTR;
};

int main() {
    Print("Model Ropovod - SIMLIB/C++\n");
    Init(0,1000);
    new Simulator();
    Run();
    return 0;
}

