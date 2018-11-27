
#include <algorithm>
#include <cstring>
#include <iostream>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "simlib.h"

#include "log.h"

const double Numeric_Const = 1.0e-03;           // prevent stupidly small numbers

typedef std::function<void(double)> Callback;

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

        void setProduction(double p) { mproduction = p;}
        double getProduction() { return mproduction; }

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
        double getMaximum() { return mmaximum; }
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

class Reserve {
    public:
        Reserve(std::string name, double capacity):
            mname(name), il(capacity), mlevel(capacity) {
        }

        double Send(double amount) {
            #ifdef RESERVE_LOG
                std::cerr << Time << ") Reserve " << mname << ": Received " << amount <<".\n";
            #endif
            mlevel += il.output(amount);
            return il.rest(amount);
        }

        double Request(double amount) {
            if(amount<=Numeric_Const) return 0;       // ignore nonsense
            if(mlevel < amount) {
                amount = mlevel;
                mlevel = 0;
                return amount;
            } else {
                mlevel -= amount;
                return amount;
            }
        }

        double getCapacity() { return il.getMaximum(); }
        double Level() { return mlevel; }

        double Missing() {
            double miss = mlevel - 900.0;
            if(miss >= 0.0) miss = 0.0;
            return -miss;
        }

    private:
        std::string mname;
        InputLimiter il;
        double mlevel;
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

struct RafineryStatus {
    std::string name;
    std::map<double,double> production;
    double maximum;
    bool broken;

    void print() {
        double val = (this->production.count(Time) > 0) ? this->production[Time] : 0;
        if(val < 0.001) val = 0;
        std::string prod = double2str(val);
        if(val == this->maximum) prod = red(prod);
        else if(val == 0) prod = red(prod);
        else prod = green(prod);

        std::cout << bold(this->name);
        for(int i = this->name.length(); i <= 10; i++) {std::cout << " ";}
        std::cout << italic("rafinery\t");
        std::cout << bold(((this->broken)?red("Broken"):green("OK"))) << "\t";
        std::cout << prod << "/" << double2str(this->maximum) << "\n";
        
    }
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

        RafineryStatus getStatus() {
            RafineryStatus rs;
            rs.name = mname;
            rs.production = getProduction();
            rs.maximum = il.getMaximum();
            rs.broken = f.IsSet();
            return rs;
        }

        std::map<double,double> getProduction() {
            std::map<double,double> clone;
            for(int i = Time; i < Time+d; i++) {
                double val = (processing.count(i) > 0)? processing[i] : 0;
                clone.insert( std::make_pair(i, val) );
            }
            return clone;
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

const double IKL_Max = 27.4;
const double Druzba_Max = 24.66;
const double Kralupy_Max = 9.04;
const double Litvinov_Max = 14.79;

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
    double Reserve = 0.0;
};
struct Demand {
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
                //output.Reserve = 0.0;
            }
            else if(Kralupy->IsBroken() && Litvinov->IsBroken()) {
                output.Kralupy = 0;
                output.Litvinov = 0;
                //output.Reserve = 1;
            }
            else if(Kralupy->IsBroken()) {
                output.Kralupy = 0;
                output.Litvinov = 1;
                //output.Reserve = Kralupy_Ratio;
            }
            else {
                output.Kralupy = 1;
                output.Litvinov = 0;
                //output.Reserve = Litvinov_Ratio;
            }

            // central receives up to two deliveries of oil per day
            // if it is a new day (first delivery), remember the amount of oil received and wait for more
            if(firstDelivery) {
                //
                oilToday = amount;
                firstDelivery = false;
                return;
            }
            else {
                oilToday += amount;
                firstDelivery = true;
                demandOil = max_3(demand.benzin/Fraction_Benzin, demand.naphta/Fraction_Naphta, demand.asphalt/Fraction_Asphalt);
            }
            // demand correction - DEMAND FIRST, RESERVE SECOND
            short KralupyBreakFlag;
            short LitvinovBreakFlag;
            KralupyBreakFlag = (Kralupy->IsBroken())?0:1;
            LitvinovBreakFlag = (Litvinov->IsBroken())?0:1;
            if(demandOil > oilToday && (demandOil-oilToday > Numeric_Const)) {
                // ask reserve for oil (only as much as the refineries will be able to process)
                if(demandOil <= Kralupy_Max*KralupyBreakFlag + Litvinov_Max*LitvinovBreakFlag) {
                    #ifdef DISTRIBUTE_LOG
                        std::cerr << Time << ") Central: Asking reserve for " << demandOil-oilToday << " units of oil.\n";
                    #endif
                    oilToday += CTR->Request(demandOil-oilToday);
                }
                else {
                    #ifdef DISTRIBUTE_LOG
                        double help = ((Kralupy_Max*KralupyBreakFlag+Litvinov_Max*LitvinovBreakFlag)-oilToday>0.0)?(Kralupy_Max*KralupyBreakFlag+Litvinov_Max*LitvinovBreakFlag)-oilToday:0.0;
                        if(help<Numeric_Const) help = 0.0;
                        std::cerr << Time << ") Central: Asking reserve for " << help << " units of oil. Demand too high.\n";
                    #endif
                    oilToday += CTR->Request((Kralupy_Max*KralupyBreakFlag+Litvinov_Max*LitvinovBreakFlag)-oilToday);
                }
                #ifdef CENTRAL_LOG
                    if(oilToday < demandOil || (Kralupy_Max*KralupyBreakFlag+Litvinov_Max*LitvinovBreakFlag)<demandOil)
                        std::cerr << Time << ") Central: Demand cannot be satisfied today, " << demandOil-oilToday << " units of oil are missing.\n";
                #endif
            }
            else {
                // reserve interactions - send
                double missing = CTR->Missing();
                double canSend = oilToday - demandOil;
                if(missing != 0.0 && canSend != 0.0) {
                    #ifdef DISTRIBUTE_LOG
                        std::cerr << Time << ") Central: Sending " << ((missing<=canSend)?missing:canSend) << " to CTR\n";
                    #endif
                    // send all that is over demand ???
                    CTR->Send((missing<=canSend)?missing:canSend);
                    oilToday = (missing<=canSend)?oilToday-missing:oilToday-canSend;
                    //double returnOil;
                    //returnOil = CTR->Send(canSend);
                    //oilToday -= (canSend-returnOil);
                }
            }
            double overflow = 0.0;

            if(oilToday*output.Kralupy > Kralupy_Max) {
                koutput(Kralupy_Max);
                overflow += oilToday*output.Kralupy - Kralupy_Max;
            }
            else
                koutput(oilToday*output.Kralupy);
            #ifdef DISTRIBUTE_LOG
                std::cerr << Time << ") Central: Sending " << oilToday*output.Kralupy-overflow << " to Kralupy.\n";
            #endif

            if(oilToday*output.Litvinov > Litvinov_Max) {
                loutput(Litvinov_Max);
                overflow += oilToday*output.Litvinov - Litvinov_Max;
            }
            else
                loutput(oilToday*output.Litvinov);
            #ifdef DISTRIBUTE_LOG
                double x = (oilToday*output.Litvinov > Litvinov_Max)?Litvinov_Max:(oilToday*output.Litvinov);
                std::cerr << Time << ") Central: Sending " << x << " to Litvinov.\n";
            #endif

            if(!output.Kralupy && !output.Litvinov)
                overflow = oilToday;
            #ifdef DISTRIBUTE_LOG
                std::cerr << Time << ") Central: Sending " << overflow << " to CTN.\n";
            #endif
            double lostOil;
            lostOil = CTR->Send(overflow);
            #ifdef DISTRIBUTE_LOG
                if(lostOil)
                    std::cerr << Time << ") Central: " << lostOil << " units of oil lost.\n";
            #endif

            // TODO: reflektovat potrebu ropy - vyslat pozadavek na ropovody (missing,demand,oilToday,input)
            double totalNeed;
            // ignores travel time -> will give reserve more than necessary (should not be a problem)
            // can be improved by mapping requests...
            double Kralupy_req = (output.Kralupy==0.0)?0.0:Kralupy_Max;
            double Litvinov_req = (output.Litvinov==0.0)?0.0:Litvinov_Max;
            totalNeed = (demandOil>(Kralupy_req+Litvinov_req))?(Kralupy_req+Litvinov_req):demandOil + CTR->Missing();
            // request for oil pipelines
            // if one oilPipeline is full, second one shares the burden
            double toIKL = 0.0;             // shift oil demand to ikl
            double toDruzba = 0.0;          // shift oil demand to druzba
            if(totalNeed*input.Druzba > Druzba_Max) toIKL = totalNeed*input.Druzba - Druzba_Max;
            if(totalNeed*input.IKL > IKL_Max) toDruzba = totalNeed*input.IKL - IKL_Max;
            #ifdef CENTRAL_LOG
                std::cerr << Time << ") Central: Setting production for Druzba to " << totalNeed*input.Druzba + toDruzba << ".\n";
            #endif
            Druzba->setProduction(totalNeed*input.Druzba + toDruzba);
            #ifdef CENTRAL_LOG
                std::cerr << Time << ") Central: Setting production for IKL to " << totalNeed*input.IKL + toIKL << ".\n";
            #endif
            IKL->setProduction(totalNeed*input.IKL + toIKL);

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
        Pipe* LitPipe;                      // oil for Litvinov is sent via this pipe
        Reserve* CTR;
        struct Demand& demand;              // current demand set by simulator
        //int lastTime = -1;                  // time of the last call of enter function
        bool firstDelivery = true;          // first delivery of this day
        double oilToday = 0;                // oil received today so far
        double demandOil = 0;               // demand of oil for today
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

            CTR = new Reserve("Nelahozeves", 1293.5
                /* sem se zapoji Central[](double amount){ std::cerr << Time << ") ControlCenter: Receive " << amount << ".\n";}
            */);

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
            int skip = 0;
            do {
                #ifdef SIMULATOR_LOG
                std::cerr << "Simulator step.\n";
                #endif
                bool newinput = false;
                bool invalid = false;
                std::string line;
                do {
                    if(skip > 0) { skip--; break; }
                    newinput = false;
                    invalid = false;
                    std::cout << "[Day " << Time << "] >> " << std::flush;
                    getline(std::cin, line);
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

                    } else if(split[0] == "skip"
                      || split[0] == "s") {
                        if(split.size() == 2) {
                            skip = std::stoi(split[1]) - 1;
                        } else {
                            invalid = true;
                        }
                    // change request
                    } else if(split[0] == "demand"
                           || split[0] == "d") {
                        newinput = true;
                        if(split.size() <= 1) { // print all request values
                            std::cout << bold("Current demand:\n");
                            std::cout << italic("- benzin: ") << demand.benzin << "\n";
                            std::cout << italic("- naphta: ") << demand.naphta << "\n";
                            std::cout << italic("- asphalt: ") << demand.asphalt << "\n\n";
                        // benzin
                        } else if(split[1] == "benzin"
                               || split[1] == "natural"
                               || split[1] == "b") {
                            if(split.size() == 2) {         // print benzin request value
                                std::cout << italic("Benzin demand: ") << demand.benzin << "\n\n";
                            } else if(split.size() == 3) {  // set benzin request value
                                demand.benzin = std::stod(split[2]);
                                std::cerr << italic("New benzin value") << " is " << demand.benzin << "\n\n";
                            } else {                        // error
                                std::cerr << "Invalid input.\n";
                                invalid = true;
                            }

                        // naphta
                        } else if(split[1] == "naphta" || split[1] == "nafta" || split[1] == "n"
                               || split[1] == "diesel" || split[1] == "d") {
                            if(split.size() == 2) {         // print naphta request value
                                std::cout << italic("Naphta demand: ") << demand.naphta << "\n\n";
                            } else if(split.size() == 3) {  // set naphta request value
                                demand.naphta = std::stod(split[2]);
                                std::cerr << italic("New naphta value") << " is " << demand.naphta << "\n\n";
                            } else {                        // error
                                std::cerr << "Invalid input.\n";
                                invalid = true;
                            }

                        // asphalt
                        } else if(split[1] == "asphalt" || split[1] == "asfalt" || split[1] == "a") {
                            if(split.size() == 2) {         // print asphalt request value
                                std::cout << italic("Asphalt demand: ") << demand.asphalt << "\n\n";
                            } else if(split.size() == 3) {  // set asphalt request value
                                demand.asphalt = std::stod(split[2]);
                                std::cerr << italic("New asphalt value") << " is " << demand.asphalt << "\n\n";
                            } else {                        // error
                                invalid = true;
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
                                if(fix) {
                                    std::cout << bold("Druzba") << " pipeline fixed.\n";
                                    Druzba->Fix();
                                } else {
                                    std::cout << bold("Druzba") << " pipeline broken.\n";
                                    Druzba->Break();
                                }
                                std::cout << "\n";
                            } else if(split[1] == "ikl" || split[1] == "i") {
                                if(fix) {
                                    std::cout << bold("IKL") << " pipeline fixed.\n";
                                    IKL->Fix();
                                } else {
                                    std::cout << bold("IKL") << " pipeline broken.\n";
                                    IKL->Break();
                                }
                                std::cout << "\n";
                            } else if(split[1] == "kralupy" || split[1] == "k") {
                                if(fix) {
                                    std::cout << bold("Kralupy") << " rafinery fixed.\n";
                                    Kralupy->Fix();
                                } else {
                                    std::cout << bold("Kralupy") << " rafinery broken.\n";
                                    Kralupy->Break();
                                }
                                std::cout << "\n";
                            } else if(split[1] == "litvinov" || split[1] == "l") {
                                if(fix) {
                                    std::cout << bold("Litvinov") << " rafinery fixed.\n";
                                    Litvinov->Fix();
                                } else {
                                    std::cout << bold("Litvinov") << " rafinery broken.\n";
                                    Litvinov->Break();
                                }
                                std::cout << "\n";
                            } else if(split[1] == "all" || split[1] == "a") {
                                if(fix) {
                                    if(Druzba->IsBroken()) { std::cout << bold("Druzba") << " pipeline fixed.\n"; Druzba->Fix();}
                                    if(IKL->IsBroken()) { std::cout << bold("IKL") << " pipeline fixed.\n"; IKL->Fix(); }
                                    if(Kralupy->IsBroken()) { std::cout << bold("Kralupy") << " rafinery fixed.\n"; Kralupy->Fix(); }
                                    if(Litvinov->IsBroken()) { std::cout << bold("Litvinov") << " rafinery fixed.\n"; Litvinov->Fix(); }
                                } else {
                                    if(!Druzba->IsBroken()) { std::cout << bold("Druzba") << " pipeline broken.\n"; Druzba->Break(); }
                                    if(!IKL->IsBroken()) { std::cout << bold("IKL") << " pipeline broken.\n"; IKL->Break(); }
                                    if(!Kralupy->IsBroken()) { std::cout << bold("Kralupy") << " rafinery broken.\n"; Kralupy->Break(); }
                                    if(!Litvinov->IsBroken()) { std::cout << bold("Litvinov") << " rafinery broken.\n"; Litvinov->Break(); }
                                }
                                std::cout << "\n";
                            } else {
                                invalid = true;
                            }
                        } else {
                            invalid = true;
                        }

                    // status
                    } else if(split[0] == "help" || split[0] == "h") {
                        newinput = true;
                        std::cout << bold("Console commands: \n");
                        std::cout << italic("Next") << "                          Count next day.\n";
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

                    } else if(split[0] == "status" || split[0] == "stat") {
                        newinput = true;
                        PipelineStatus druzbastat = Druzba->getStatus();
                        PipelineStatus iklstat = IKL->getStatus();
                        RafineryStatus kralupystat = Kralupy->getStatus();
                        RafineryStatus litvinovstat = Litvinov->getStatus();
                        if(split.size() > 1) {
                            if(split[1] == "druzba" || split[1] == "druzhba" || split[1] == "d") {
                                druzbastat.print();
                                for(auto& it: druzbastat.delivery) {
                                    if(it.second > 0.001) {
                                        std::cout << "Time " << it.first << ": " << it.second << "\n";
                                    }
                                }
                                std::cout << "\n";
                            } else if(split[1] == "ikl" || split[1] == "i") {
                                iklstat.print();
                                for(auto& it: iklstat.delivery) {
                                    if(it.second > 0.001) {
                                        std::cout << "Time " << it.first << ": " << it.second << "\n";
                                    }
                                }
                                std::cout << "\n";
                            } else if(split[1] == "kralupy" || split[1] == "k") {
                                kralupystat.print();
                                for(auto& it: kralupystat.production) {
                                    if(it.second > 0.001) {
                                        std::cout << "Time " << it.first << ": " << it.second << "\n";
                                    }
                                }
                                std::cout << "\n";
                            } else if(split[1] == "litvinov" || split[1] == "l") {
                                litvinovstat.print();
                                for(auto& it: litvinovstat.production) {
                                    if(it.second > 0.001) {
                                        std::cout << "Time " << it.first << ": " << it.second << "\n";
                                    }
                                }
                                std::cout << "\n";
                            } else {
                                invalid = true;
                            }
                        } else {
                            druzbastat.print();
                            iklstat.print();
                            kralupystat.print();
                            litvinovstat.print();
                            std::cout << "\n";
                        }

                    // exit
                    } else if(split[0] == "quit" || split[0] == "exit" || split[0] == "q") {
                        std::cout << bold("Quit.\n");
                        exit(0);

                    // unknown command
                    } else {
                        invalid = true;
                    }
                    if(!invalid) mem = line;
                    else { std::cerr << red("Invalid input.\n"); }
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
    std::cout << style("Model Ropovod - SIMLIB/C++\n", BOLD);
    Init(1,365);
    new Simulator();
    Run();
    return 0;
}
