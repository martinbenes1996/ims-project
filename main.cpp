
#include <iostream>
#include <functional>
#include <queue>
#include <string>

#include "simlib.h"


class PetroleumChunk: public Event {
    public:
        PetroleumChunk(std::function<void(long)> f, long amount): mf(f), mamount(amount) {}
        void Behavior() { 
            std::cerr << Time << ") PetroleumChunk: " << mamount << " arrived.\n";
            mf(mamount);
            Cancel();
        }
    private:
        std::function<void(long)> mf;
        long mamount;
};

class Pipe {
    public:
        Pipe(double delay, long maximum):
            mdelay(delay), mmaximum(maximum) {}
        void setCallback(std::function<void(long)> f) { getCallback() = f; }

        long Transport(long amount) {
            // get amount to send and amount to leave behind
            long send, rest, available = mmaximum - CurrentlyTransporting();
            if(available > 0) {
                if(available > amount) {
                    send = amount;
                    rest = 0;
                } else {
                    send = available;
                    rest = amount - available;
                }
            } else {
                send = 0;
                rest = amount;
            }
            //
            std::cerr << Time << ") Pipe: Request to send " << send << ", " << amount-rest << " sent.\n";

            // send petroleum
            if(send > 0) {
                PetroleumChunk* chunk = new PetroleumChunk(getCallback(), send);
                chunk->Activate(Time+mdelay);
            }
            mcurrentlytransporting += send;
            
            return rest;
        }

    private:
        double mdelay;
        long mmaximum;
        
        std::function<void(long)>& getCallback() {
            static std::function<void(long)> f;
            return f;
        }

        long mcurrentlytransporting = 0;
        long CurrentlyTransporting() { return mcurrentlytransporting; }
};

class Source : public Process {
    public:
        Source(long production): mproduction(production) {
            Activate(Time);
        }
        void setCallback(std::function<void(long)> f) { mf = f; }
        void Behavior() {
            std::cerr << Time << ") Source: Produced " << mproduction << ".\n";
            mf(mproduction);
            std::cerr << Time << ") Source: Activate again in " << Time+1 << ".\n";
            Activate(Time + 1);
        }
    private:
        long mproduction;
        std::function<void(long)> mf;
};

class Ropovod {
    public:
        Ropovod(const std::string& name, Source* source, Pipe* pipe):
            msource(source), mpipe(pipe), mname(name) {
            // production of source is directed here
            msource->setCallback( [this](long amount){ this->Transport(amount); } );
            mpipe->setCallback( [](long){ std::cerr << "Here!\n"; } );
        }
    private:
        Source *msource;
        Pipe *mpipe;
        std::string mname;

        long mwaiting = 0;
        void Transport(long petroleum) {
            mwaiting = mpipe->Transport(petroleum + mwaiting); 
            // log here
        }
};

int main() {
    Print("Model Ropovod - SIMLIB/C++\n");
    Init(0,1000);
    Ropovod("Druzhba", new Source(100), new Pipe(2,200));
    Run();
    return 0;
}
