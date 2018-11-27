
/**
 * @file pipeline.cpp
 * @authors xbenes49 xpolan09
 * @date 27th november 2018
 * @brief Pipeline classes definitions.
 *
 * This module implements classes Source, Pipe and Pipeline and its relatives.
 */


#include "pipeline.h"


void Source::Behavior() {
    do {
        // wait for command line
        bool seize = (Time >= EventOrder.t);
        if(seize) Seize(EventOrder.waitForConsole);

        #ifdef SOURCE_LOG
            std::cerr << Time << ") Source " << mname << ": Produced " << mproduction << ".\n";
        #endif
        // output
        moutput(mproduction);

        // end waiting
        if(seize) Release(EventOrder.waitForConsole);
        Wait(1);
    } while(true);
}

void Transfer::Behavior() {
    // wait for command line
    bool seize = (Time >= EventOrder.t);
    if(seize) {
        Seize(EventOrder.waitForConsole);
    }
    
    #ifdef TRANSFER_LOG
        std::cerr << Time << ") Transfer: transferred " << mamount << ".\n";
    #endif
    // output
    moutput(mamount);
    // end waiting
    if(seize) {
        Release(EventOrder.waitForConsole);
    }
}

void Pipe::Send(double amount) {
    // check maximums
    amount = f.Check(amount);
    PlanSending(amount, Time);
    if(sending.count(Time + d) == 0) {
        amount = 0;
    }

    #ifdef PIPES_LOG
        std::cerr << Time << ") Pipe " << mname << ": Sending " << sending[Time+d] << ".\n";
    #endif
    // send
    (new Transfer(sending[Time+d], getOutput()))->Activate(Time + d);
}


std::map<double,double> Pipe::getCurrentFlow() {
    // clone flow
    std::map<double,double> clone;
    for(int i = Time; i < Time+d; i++) {
        double val = (sending.count(i) > 0) ? sending[i] : 0;
        clone.insert( std::make_pair(i, val) );
    }
    return clone;
}


void Pipe::PlanSending(double amount, double t) {
    if(amount == 0) return;
    // sum sending part
    double sum = 0;
    for(int i = t; i <= (t+d); i++) {
        if(sending.count(i)) sum += sending[i];
    }
    if((sum+amount) > maxStorage) {
        amount = maxStorage - sum;
    }
    // send
    sending[t+d] += il.output(amount);
    // recursive for rest
    PlanSending(il.rest(amount), t+1);
}


void PipelineStatus::print() {
    // production > string
    std::string prod = double2str(this->production);
    if(this->production == this->maximum) prod = red(prod);
    else if(this->production == 0) prod = red(prod);
    else prod = green(prod);
    // print
    std::cout << bold(this->name);
    for(int i = this->name.length(); i <= 10; i++) {std::cout << " ";}
    std::cout << italic("pipeline\t");
    std::cout << bold(((this->broken)?red("Broken"):green("OK"))) << "\t";
    std::cout << prod << "/" << double2str(this->maximum) << "\n";
}


OilPipeline::OilPipeline(std::string name, double maxProduction, double producing, double delay):
    mname(name), mmaximum(maxProduction), mdelay(delay) {
    // create pipe
    p = new Pipe(mname, mmaximum, mdelay, getOutput());
    // create source
    s = new Source(mname, producing, p->getInput());
    s->Activate();

    // generate initial transactions
    std::map<double,double> plan;
    for(int i = 0; i < delay; i++) {
        (new Transfer(producing, p->getOutput()))->Activate(Time+i);
        plan.insert( std::make_pair(Time+i, producing) );
    }
    // create plan for pipe
    p->setSending(plan);
}

void OilPipeline::Output(double amount) {
    #ifdef PIPELINE_LOG
        std::cerr << Time << ") OilPipeline " << mname << ": Received " << amount << "\n";
    #endif
    // output amount
    moutput(amount);
}

PipelineStatus OilPipeline::getStatus() {
    PipelineStatus ps;
    ps.name = mname;
    ps.delivery = p->getCurrentFlow();
    ps.production = s->getProduction();
    ps.maximum = mmaximum;
    ps.broken = p->IsBroken();
    return ps;
}