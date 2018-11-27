
#include "rafinery.h"


void RafineryStatus::print() {
    // production to string
    double val = (this->production.count(Time) > 0) ? this->production[Time] : 0;
    if(val < 0.001) val = 0;
    std::string prod = double2str(val);
    if(val == this->maximum) prod = red(prod);
    else if(val == 0) prod = red(prod);
    else prod = green(prod);
    // print
    std::cout << bold(this->name);
    for(int i = this->name.length(); i <= 10; i++) {std::cout << " ";}
    std::cout << italic("rafinery\t");
    std::cout << bold(((this->broken)?red("Broken"):green("OK"))) << "\t";
    std::cout << prod << "/" << double2str(this->maximum) << "\n";
}


Products FractionalDestillation::Destillate(double amount) {
    Products p;
    p.benzin = 0.19*amount;
    p.naphta = 0.42*amount;
    p.asphalt = 0.13*amount;
    return p;
}


void Rafinery::Enter(double amount) {
    amount = f.Check(amount);
    PlanProcessing(amount, Time);

    if(processing.count(Time) > 0) {
        #ifdef RAFINERY_LOG
            std::cerr << Time << ") Rafinery " << mname << ": Process " << amount << ".\n";
        #endif
        (new FractionalDestillation(processing[Time], getOutput()))->Activate(Time+d);
    }
}

RafineryStatus Rafinery::getStatus() {
    RafineryStatus rs;
    rs.name = mname;
    rs.production = getProduction();
    rs.maximum = il.getMaximum();
    rs.broken = f.IsSet();
    return rs;
}

std::map<double,double> Rafinery::getProduction() {
    std::map<double,double> clone;
    for(int i = Time; i < Time+d; i++) {
        double val = (processing.count(i) > 0)? processing[i] : 0;
        clone.insert( std::make_pair(i, val) );
    }
    return clone;
}

void Rafinery::PlanProcessing(double amount, double t) {
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
    processing[t+d] += il.output(amount);
    PlanProcessing(il.rest(amount), t+1);
}