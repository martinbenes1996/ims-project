# IMS Project
This is project for Modelling and simulation subject.

# Usage
First, you need to install simlib library. It is included here.

- $ cd simlib
- $ make
- $ sudo make install

Then compile in the main directory with make.

# Console
The whole model is controlled via console. User can manage it
with following commands

- next/n *computes another day*
- demand/d [comodity [<number>]] *manages demand*

Examples:
- demand/d*shows current demand*
- demand/d benzin/b *shows current demand for benzin*
- demand/d naphta/nafta/diesel/n/d *shows current demand for diesel*
- demand/d asphalt/asfalt/a *shows current demand for asfalt*
- demand/d benzin/natural/b <number> *sets demand for benzin to number*
- demand/d naphta/nafta/diesel/n/d <number> *sets demand for diesel to number*
- demand/d asphalt/asfalt/a <number> *sets demand for asfalt to number*



