#ifndef ARDUINO_BACNET_CONFIGS_H
#define ARDUINO_BACNET_CONFIGS_H


//MSTP configuration
#define BACDL_MSTP

// for confirmed messages, this is the number of transactions
// that we hold in a queue waiting for timeout.
// Configure to zero if you don't want any confirmed messages
// Configure from 1..255 for number of outstanding confirmed
// requests available.
//Get low for les RAM usage
//255	Maximum/full-size table; huge RAM cost
//127	Plenty for a fairly active client
//31	Good embedded default
//16	Probably plenty for many controllers
//8	Very reasonable small controller
//4	Fine if you have little simultaneous client activity
//2	Possible for a very constrained device
//1	Minimum practical value
#define MAX_TSM_TRANSACTIONS 31


#define MAX_ADDRESS_CACHE 63


#endif // ARDUINO_BACNET_CONFIGS_H