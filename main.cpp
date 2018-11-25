
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

class OilPipeline {
    public:
        OilPipeline(double delay, double maxProduction, double producing):
            mdelay(delay), mmaximum(maxProduction), mproducing(producing) {
            
            p = new Pipe(mdelay, mmaximum, getOutput());
            s = new Source(producing, p->getInput());
            s->Activate(Time);
        }
        ~OilPipeline() { std::cerr << Time << ") OilPipeling: Destroy itself.\n"; }
        void Foo(double amount) {
            std::cerr << Time << ") OilPipeline: Received " << amount << "\n";
            moutput(amount);
        }
        Callback getOutput() { return [this](double amount){ this->Foo(amount); }; }

        void setOutput(Callback output) { moutput = output; }
        
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

int main() {
    Print("Model Ropovod - SIMLIB/C++\n");
    Init(0,1000);
    //Ropovod("Druzhba", new Source(100), new Pipe(2,200));
    //Rafinery("Kralupy", 9.04, [](Products){}, 0);
    OilPipeline *druzba = new OilPipeline(2, 200, 100);
    Rafinery *kralupy = new Rafinery(5, 200);
    druzba->setOutput( kralupy->getInput() );
    Run();
    return 0;
}

