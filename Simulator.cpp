#include <fstream>
#include <iostream>
#include <sstream>

#include <windows.h>
#include <boost/thread.hpp>
#include <Synchapi.h>

#include "Agent.h"
#include "Market.h"
#include "Order.h"
#include "OrderBook.h"
#include "DistributionUniform.h"
#include "DistributionGaussian.h"
#include "DistributionExponential.h"
#include "DistributionConstant.h"
#include "LiquidityProvider.h"
#include "NoiseTrader.h"
#include "Exceptions.h"
#include "Plot.h"
#include "Stats.h"
#include "RandomNumberGenerator.h"




void plotOrderBook(Market *aMarket,Plot* aplotter,int a_orderBookId)
{
	std::vector<int> price;
	std::vector<int> priceQ;
	int last;
	aMarket->getOrderBook(a_orderBookId)->getOrderBookForPlot(price,priceQ);
	last = aMarket->getOrderBook(a_orderBookId)->getPrice();
	aplotter->plotOrderBook(price,priceQ,last);
}

int nbAssets = 1;
int storedDepth = 0;

// Parameters for the liquidity provider
double meanActionTimeLP = 0.35;
// int meanVolumeLP = 100;
int meanPriceLagLP = 6;
double buyFrequencyLP = 0.25;
double cancelBuyFrequencyLP = 0.25;
double cancelSellFrequencyLP = 0.25;
double uniformCancellationProbability = 0.01;

// Parameters for the liquidity takers : NT and LOT
double meanDeltaTimeMarketOrder = 2.2 ;
double percentageLargeOrders = 0.01 ;

double meanActionTimeNT = meanDeltaTimeMarketOrder / (1.0-percentageLargeOrders) ;
int meanVolumeNT = 100;
double buyFrequencyNT = 0.5;

//if we use a uniform distribution
int maxVolumeNT = 500;

double meanActionTimeLOT = meanDeltaTimeMarketOrder / percentageLargeOrders ;
int meanVolumeLOT = 100;
double buyFrequencyLOT = 0.5;

int nInitialOrders = 1 ;
double simulationTimeStart = 0 ;
double simulationTimeStop = 1000;
double printIntervals = 30; //900 ;
double impactMeasureLength = 60 ;


boost::mutex a;

void agentThread(Market *myMarket, Agent *actingAgent, double *currentTime, int sleepTime, std::string threadName)
{
	try{
		while(*currentTime<simulationTimeStop)
		{
			// Get next time of action
			a.lock();
			*currentTime += actingAgent->getNextActionTime() ;
			a.unlock();
			// Select next player
			//Agent * actingAgent = myMarket->getNextActor() ;
			// Submit order
			actingAgent->makeAction( actingAgent->getTargetedStock(), *currentTime) ;
			// From time to time, check state of order book
			// Update clock
			myMarket->setNextActionTime() ;
		//	std::cout << threadName << "   currentTime = " << *currentTime << std::endl ;	
			Sleep(sleepTime);
		}
	}
	catch(Exception &e)
	{
		std::cout <<e.what()<< std::endl ;
	}
}


int main(int argc, char* argv[])
{
	Plot * plotter = new Plot() ;
	std::ostringstream oss_marketName ;
	oss_marketName << "LargeTrader_" << percentageLargeOrders ;
	Market *myMarket = new Market(oss_marketName.str());
	myMarket->createAssets(nbAssets);

	myMarket->getOrderBook(1)->setStoreOrderBookHistory(true,1);
	myMarket->getOrderBook(1)->setStoreOrderHistory(true);
	//myMarket->getOrderBook(1)->setPrintOrderBookHistory(true,storedDepth);
	RandomNumberGenerator * myRNG = new RandomNumberGenerator();

	// Create one Liquidity Provider
	DistributionExponential * LimitOrderActionTimeDistribution = new DistributionExponential(myRNG, meanActionTimeLP) ;

	//DistributionExponential *  LimitOrderOrderVolumeDistribution = new DistributionExponential(myRNG, meanVolumeLP) ;
	DistributionGaussian *  LimitOrderOrderVolumeDistribution = new DistributionGaussian(myRNG, 0.7*maxVolumeNT,sqrt(0.2*maxVolumeNT)) ;
	
	//DistributionConstant * LimitOrderOrderVolumeDistribution = new DistributionConstant(myRNG, meanVolumeLP) ;
	DistributionExponential * LimitOrderOrderPriceDistribution = new DistributionExponential(myRNG, meanPriceLagLP) ;
	LiquidityProvider * myLiquidityProvider = new LiquidityProvider
		(
		myMarket, 
		LimitOrderActionTimeDistribution,
		LimitOrderOrderVolumeDistribution,
		LimitOrderOrderPriceDistribution,
		buyFrequencyLP,
		1,
		cancelBuyFrequencyLP,
		cancelSellFrequencyLP,
		uniformCancellationProbability
		) ;
	myMarket->registerAgent(myLiquidityProvider);

	// Submit nInitialOrders limit orders to initialize order book
	for(int n=0; n<nInitialOrders; n++)
	{
		myLiquidityProvider->makeAction(1, 0.0) ;	
	}
	std::cout 
		<< "Time 0 : [bid ; ask] = " 
		<< "[" << myMarket->getOrderBook(1)->getBidPrice()/100.0 << " ; "
		<< myMarket->getOrderBook(1)->getAskPrice()/100.0 << "]"
		<< "  [sizeBid ; sizeAsk] = " 
		<< "[" << myMarket->getOrderBook(1)->getTotalBidQuantity() << " ; "
		<< myMarket->getOrderBook(1)->getTotalAskQuantity() << "]"
		<< std::endl ;
	std::cout << "Order book initialized." << std::endl ;
	plotOrderBook(myMarket,plotter,1);

	// Create one Noise Trader
	DistributionExponential * NoiseTraderActionTimeDistribution = new DistributionExponential(myRNG, meanActionTimeNT) ;
	DistributionUniform * NoiseTraderOrderTypeDistribution = new DistributionUniform(myRNG) ;
	
	DistributionUniform * NoiseTraderOrderVolumeDistribution = new DistributionUniform(myRNG, 0, maxVolumeNT) ;
	//DistributionExponential * NoiseTraderOrderVolumeDistribution = new DistributionExponential(myRNG, meanVolumeNT) ;
	//DistributionConstant * NoiseTraderOrderVolumeDistribution = new DistributionConstant(myRNG, meanVolumeNT) ;
	NoiseTrader * myNoiseTrader = new NoiseTrader(myMarket, 
		NoiseTraderActionTimeDistribution,
		NoiseTraderOrderTypeDistribution,
		NoiseTraderOrderVolumeDistribution,
		buyFrequencyNT,1) ;
	myMarket->registerAgent(myNoiseTrader);

	// Simulate market
	std::cout << "Simulation starts. " << std::endl ;
	double currentTime = simulationTimeStart ;
	double *currentTimePtr = &currentTime;

	int i=1;

	//std::vector<double>  MidPriceTimeseries ;
	//boost::thread threadNoise(agentThread, myMarket, myNoiseTrader, currentTime, 5000, "noise");
	//boost::thread threadLiquidity(agentThread, myMarket, myLiquidityProvider, currentTime, 3000, "liquidity");

	boost::thread_group agentGroup;

	agentGroup.create_thread(boost::bind(&agentThread, myMarket, myNoiseTrader, currentTimePtr, 300, "noise   "));
	agentGroup.create_thread(boost::bind(&agentThread, myMarket, myLiquidityProvider, currentTimePtr, 100, "liquidity"));
//		agentThread(Market *myMarket, Agent *actingAgent, double currentTime)

	try{
		while(*currentTimePtr<simulationTimeStop)
		{
			//std::cout << "currentTime main = " << *currentTimePtr << "  i * print interval = " << i*printIntervals << std::endl ;
			if(*currentTimePtr>i*printIntervals)
			{
				std::cout
					<<"time: "<<*currentTimePtr << std::endl
					<< "[bid;ask]=" 
					<< "[" << myMarket->getOrderBook(1)->getBidPrice()/100.0 << " ; "
					<< myMarket->getOrderBook(1)->getAskPrice()/100.0 << "]"<< std::endl
					<< "[sizeBid;sizeAsk]=" 
					<< "[" << myMarket->getOrderBook(1)->getTotalBidQuantity()<<";" <<
					+ myMarket->getOrderBook(1)->getTotalAskQuantity() << "]"
					<< std::endl
					<< "[pendingLP;pendingNT]="
					<< "[" << myLiquidityProvider->getPendingOrders()->size()<<";"
					<< myNoiseTrader->getPendingOrders()->size() << "]"
					<< std::endl;
				// Plot order book
				
				// Agents'portfolios
				std::cout << "LP: nStock=\t" << myLiquidityProvider->getStockQuantity(1) 
					<< "\t Cash=\t" << myLiquidityProvider->getNetCashPosition() << std::endl ;				
				std::cout << "NT: nStock=\t" << myNoiseTrader->getStockQuantity(1) 
					<< "\t Cash=\t" << myNoiseTrader->getNetCashPosition() << std::endl<< std::endl<< std::endl ;
				plotOrderBook(myMarket,plotter,1);
				// Update sampling
				i++;
				//std::cout << "currentTime main = " << *currentTimePtr << std::endl ;	
			}
			//std::cout << "currentTime main = " << currentTime << std::endl ;	
		}
	}
	catch(Exception &e)
	{
		std::cout <<e.what()<< std::endl ;
	}



	//join all threads
	agentGroup.join_all();
	//threadNoise.join();
	//threadLiquidity.join();
	std::cout << "join done" << std::endl ;
	int lol;
	std::cin>>lol;

	
	// Print stored history
	myMarket->getOrderBook(1)->printStoredOrderBookHistory();

	// Stats on mid price
	Stats * myStats = new Stats(myMarket->getOrderBook(1)) ;
	for(int samplingPeriod = 15 ; samplingPeriod<=960; samplingPeriod*=2)
	{
		myStats->printTimeSeries(MID,samplingPeriod) ;
		std::vector<double> midPrice = myStats->getPriceTimeSeries(MID,samplingPeriod) ;
		std::vector<double> MidPriceLogReturns ;
		for(unsigned int k=1; k<midPrice.size() ; k++){
			MidPriceLogReturns.push_back(log(midPrice[k])-log(midPrice[k-1])) ;
		}
		std::ostringstream dataLabel ;
		dataLabel << "MidPrice_" << samplingPeriod << "_LogReturns" ;
		myStats->plotPDF(dataLabel.str().c_str(),MidPriceLogReturns) ;
		myStats->plotNormalizedPDF(dataLabel.str().c_str(),MidPriceLogReturns) ;
		myStats->printAutocorrelation(dataLabel.str(), MidPriceLogReturns,(int) MidPriceLogReturns.size()/2, 1) ;
	}

	// Stats on spread
	std::vector<double> spreadTimeSeries = myStats->getPriceTimeSeries(SPREAD,0.5) ;
	myStats->plotPDF("Spread",spreadTimeSeries) ;
	//myStats->printAutocorrelation("Spread", spreadTimeSeries, 57600, 1) ;

	// Stats on order signs of market orders
	std::vector<double> orderSigns = myStats->getOrderSignsTimeSeries(MARKET) ;
	std::ostringstream ss_tag ; ss_tag << "MarketOrders" ;
	myStats->printAutocorrelation(ss_tag.str(), orderSigns, (int)orderSigns.size()/2, 1) ;

	// Stats summary
	myStats->printSummary() ;

	// End of program
	delete myStats ;
	delete myMarket ;
	std::cout << "All done." << std::endl ;

	std::cin>>lol;
	return 0;
}
