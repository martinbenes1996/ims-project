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

Comodity might be one of following three

- *benzin*: benzin / b 
- *diesel*: naphta / nafta / diesel / d / n 
- *asphalt*: asphalt / asfalt / a

Number, as a third parameter, is set as new value for demand
of the chosen comodity, for example *demand benzin 5*.




