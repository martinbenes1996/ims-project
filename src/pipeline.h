#ifndef PIPELINE_H
#define PIPELINE_H

#include <map>
#include <string>

#include "simlib.h"

#include "tools.h"

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

        void setProduction(double p) { mproduction = p;}
        double getProduction() { return mproduction; }

    private:
        std::string mname;
        double mproduction;
        Callback moutput;

        long id;
        long& getCnt() { static long i = 1; return i; }
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
            mname(name), il(maximum), d(delay), moutput(output) {}
        void Send(double amount) {
            amount = f.Check(amount);
            PlanSending(amount, Time);

            if(sending.count(Time + d) == 0) {
                amount = 0;
            }

            #ifdef PIPES_LOG
                std::cerr << Time << ") Pipe " << mname << ": Sending " << sending[Time+d] << ".\n";
            #endif
            (new Transfer(sending[Time+d], getOutput()))->Activate(Time + d);

        }
        Callback getInput() { return [this](double amount){ this->Send(amount);}; }
        void setOutput(Callback output) { moutput = output; }

        Callback getOutput() {
            return [this](double amount){
                sending.erase(Time);
                return this->moutput(amount);
            };
        }

        void Break() { f.Set(); }
        void Fix() { f.Reset(); }
        bool IsBroken() { return f.IsSet(); }

        std::map<double,double> getCurrentFlow() {
            std::map<double,double> clone;
            for(int i = Time; i < Time+d; i++) {
                double val = (sending.count(i) > 0) ? sending[i] : 0;
                clone.insert( std::make_pair(i, val) );
            }
            return clone;
        }

        void setSending(std::map<double,double> s) {
            sending = s;
        }

    protected:
        void PlanSending(double amount, double t) {
            if(amount == 0) return;
            double sum = 0;
            for(int i = t; i <= (t+d); i++) {
                if(sending.count(i)) sum += sending[i];
            }
            if((sum+amount) > maxStorage) {
                amount = maxStorage - sum;
            }
            sending[t+d] += il.output(amount);
            PlanSending(il.rest(amount), t+1);
        }

        //double output(double amount, double basis=0) { return (amount < (mmaximum-basis))?amount:(mmaximum-basis); }
        //double rest(double amount, double basis=0) { return (amount < (mmaximum-basis))?0:(amount-mmaximum+basis); }

    private:
        std::string mname;
        InputLimiter il;
        double d;
        Callback moutput;

        double maxStorage = 100;
        std::map<double, double> sending;

        Flagger f;
};


struct PipelineStatus {
    std::string name;
    std::map<double,double> delivery;
    double production;
    double maximum;
    bool broken;

    void print() {
        std::string prod = double2str(this->production);
        if(this->production == this->maximum) prod = red(prod);
        else if(this->production == 0) prod = red(prod);
        else prod = green(prod);

        std::cout << bold(this->name);
        for(int i = this->name.length(); i <= 10; i++) {std::cout << " ";}
        std::cout << italic("pipeline\t");
        std::cout << bold(((this->broken)?red("Broken"):green("OK"))) << "\t";
        std::cout << prod << "/" << double2str(this->maximum) << "\n";
    }
};

class OilPipeline {
    public:
        OilPipeline(std::string name, double maxProduction, double producing, double delay):
            mname(name), mmaximum(maxProduction), mdelay(delay) {

            p = new Pipe(mname, mmaximum, mdelay, getOutput());

            s = new Source(mname, producing, p->getInput());
            s->Activate();

            std::map<double,double> plan;
            for(int i = 0; i < delay; i++) {
                (new Transfer(producing, p->getOutput()))->Activate(Time+i);
                plan.insert( std::make_pair(Time+i, producing) );
            }
            p->setSending(plan);
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
        void setProduction(double production) { s->setProduction(production); }

        PipelineStatus getStatus() {
            PipelineStatus ps;
            ps.name = mname;
            ps.delivery = p->getCurrentFlow();
            ps.production = s->getProduction();
            ps.maximum = mmaximum;
            ps.broken = p->IsBroken();
            return ps;
        }
        

    private:
        std::string mname;
        double mmaximum;
        double mdelay;
        bool broken = false;

        Callback moutput;

        Pipe* p;
        Source* s;
};


#endif // PIPELINE_H