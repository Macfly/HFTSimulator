#include <fstream>
#include <iostream>
#include <sstream>
#include "Agent.h"
#include "Market.h"
#include "OrderBook.h"
#include "DistributionUniform.h"
#include "DistributionGaussian.h"
#include "DistributionExponential.h"
#include "LiquidityProvider.h"
#include "NoiseTrader.h"
#include "MarketMaker.h"
#include "Exceptions.h"
#include "Plot.h"
#include "RandomNumberGenerator.h"


void plotOrderBook(Market* aMarket, Plot* aplotter, int a_orderBookId)
{
	std::vector<int> price;
	std::vector<int> priceQ;

	std::vector<int> pricesMM;
	std::vector<int> volumesMM;

	int last;

	aMarket->getOrderBook(a_orderBookId)->getOrderBookForPlot(price, priceQ, pricesMM, volumesMM);

	last = aMarket->getOrderBook(a_orderBookId)->getPrice();

	//std::vector<int> historicPrices = aMarket->getOrderBook(a_orderBookId)->getHistoricPrices();
	std::vector<double> transactionsTimes = aMarket->getOrderBook(a_orderBookId)->getTransactionsTimes();

	//int sizePrices = historicPrices.size();
	int sizeTransactionsTimes = transactionsTimes.size();

	//std::cout<<sizePrices<<std::endl;
	/*for (int k=0;k<sizePrices;k++){
					int a;
					std::cout<< aMarket->getOrderBook(1)->getHistoricPrices()[k]<<std::endl;
					std::cin>>a;
				}*/
	double variance = 0;
	//if (sizePrices>1){

	//	for (int z=0;z<(sizePrices-1);z++){
	//		//std::cout<<historicPrices[z+1]<<std::endl;
	//		//std::cout<<historicPrices[z]<<std::endl;
	//		//int a;
	//		//std::cin>>a;
	//	
	//		variance +=  pow(double( double( double(historicPrices[z+1])-double(historicPrices[z]))/double(historicPrices[z])),2) ;
	//		
	//	}
	//}


	std::cout << "Transaction Time = " << transactionsTimes[sizeTransactionsTimes - 1] / 100.0 << std::endl;

	if (transactionsTimes[sizeTransactionsTimes - 1] != 0)
	{
		double annualTime = ((((transactionsTimes[sizeTransactionsTimes - 1] / 1000.0) / 60) / 60) / 24) / 365;
		variance = aMarket->getOrderBook(a_orderBookId)->getReturnsSumSquared() / annualTime;
	}
	double volatility = pow(variance, 0.5);

	std::cout << "vol = " << volatility << std::endl;
	aplotter->plotOrderBook(price, priceQ, last, volatility, pricesMM, volumesMM);
}

int nbAssets = 1;
int storedDepth = 1;

// Parameters for the liquidity provider
double meanActionTimeLP = 0.35;
int meanVolumeLP = 100;
int meanPriceLagLP = 6;
double buyFrequencyLP = 0.5;
double cancelBuyFrequencyLP = 0;
double cancelSellFrequencyLP = 0;
double uniformCancellationProbability = 0.01;

// Parameters for the liquidity takers : NT and LOT
double meanDeltaTimeMarketOrder = 2.2;
double percentageLargeOrders = 0.01;

double meanActionTimeNT = meanDeltaTimeMarketOrder / (1.0 - percentageLargeOrders);
int meanVolumeNT = 100;
double buyFrequencyNT = 0.5;

//if you use a uniform distribution
int minVolumeNT = 50;
int maxVolumeNT = 180;

double meanActionTimeLOT = meanDeltaTimeMarketOrder / percentageLargeOrders;
int meanVolumeLOT = 1000;
double buyFrequencyLOT = 0.5;

/*double meanActionTimeTF = 10 ;
int meanVolumeTF = 1 ;

double meanActionTimeFVT = 60 ;
int meanVolumeFVT = 1 ;
int fundamentalValueFVT = 10000 ;
*/

int nInitialOrders = 1;
double simulationTimeStart = 0;
double simulationTimeStop = 188 * 10;
double printIntervals = 30; //900 ;
double impactMeasureLength = 60;

//int main()
//{
//	
//	std::ofstream outFile("zzzzz.csv");
//	RandomNumberGenerator *rng=new RandomNumberGenerator();
//	DistributionUniform * unif=new DistributionUniform(rng);
//	DistributionGaussian *normal = new DistributionGaussian(rng);
//	DistributionExponential *expd = new DistributionExponential(rng,0.2);
//	for(int i=0;i<1000;i++)
//	{
//		//std::cout<<expd->nextRandom()<<std::endl;
//		outFile<<expd->nextRandom()<<'\n';
//	}
//	outFile.close();
//	int a=2;
//	//std::cin>>a;
//
//}
int main(int argc, char* argv[])
{
	Plot* plotter2 = new Plot();
	std::vector<double> transactionsTimes;
	std::vector<int> historicPrices;
	Plot* plotter = new Plot();
	std::ostringstream oss_marketName;
	oss_marketName << "LargeTrader_" << percentageLargeOrders ;
	Market* myMarket = new Market(oss_marketName.str());
	myMarket->createAssets(nbAssets);

	myMarket->getOrderBook(1)->setStoreOrderBookHistory(true, storedDepth);
	myMarket->getOrderBook(1)->setStoreOrderHistory(true);
	//myMarket->getOrderBook(1)->setPrintOrderBookHistory(true,storedDepth);
	RandomNumberGenerator* myRNG = new RandomNumberGenerator();

	// Create one Liquidity Provider
	DistributionExponential* LimitOrderActionTimeDistribution = new DistributionExponential(myRNG, meanActionTimeLP);
	//DistributionExponential *  LimitOrderOrderVolumeDistribution = new DistributionExponential(myRNG, meanVolumeLP) ;
	DistributionGaussian* LimitOrderOrderVolumeDistribution = new DistributionGaussian(myRNG, 0.7 * 100, sqrt(0.2 * 100));
	//DistributionConstant * LimitOrderOrderVolumeDistribution = new DistributionConstant(myRNG, meanVolumeLP) ;
	DistributionExponential* LimitOrderOrderPriceDistribution = new DistributionExponential(myRNG, meanPriceLagLP);
	LiquidityProvider* myLiquidityProvider = new LiquidityProvider
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
			); 
	myMarket->registerAgent(myLiquidityProvider);

	// Create one Market Maker
	DistributionExponential* MarketOrderActionTimeDistribution = new DistributionExponential(myRNG, meanActionTimeLP);
	//DistributionExponential *  LimitOrderOrderVolumeDistribution = new DistributionExponential(myRNG, meanVolumeLP) ;
	DistributionGaussian* MarketOrderOrderVolumeDistribution = new DistributionGaussian(myRNG, 0.4 * 100, sqrt(0.2 * 100));
	//DistributionConstant * LimitOrderOrderVolumeDistribution = new DistributionConstant(myRNG, meanVolumeLP) ;
	DistributionExponential* MarketOrderOrderPriceDistribution = new DistributionExponential(myRNG, meanPriceLagLP);
	MarketMaker* myMarketMaker = new MarketMaker
		(
			myMarket,
			MarketOrderActionTimeDistribution,
			MarketOrderOrderVolumeDistribution,
			MarketOrderOrderPriceDistribution,
			buyFrequencyLP,
			1,
			cancelBuyFrequencyLP,
			cancelSellFrequencyLP,
			uniformCancellationProbability,
			0.1
		);
	myMarket->registerAgent(myMarketMaker);

	// Submit nInitialOrders limit orders to initialize order book
	for (int n = 0; n < nInitialOrders; n++)
	{
		myLiquidityProvider->makeAction(1, 0.0) ;
		std::cout << "initial order nb " << n + 1 << " out of " << nInitialOrders << std::endl;
	}
	std::cout
		<< "Time 0 : [bid ; ask] = "
		<< "[" << myMarket->getOrderBook(1)->getBidPrice() / 100.0 << " ; "
		<< myMarket->getOrderBook(1)->getAskPrice() / 100.0 << "]"
		<< "  [sizeBid ; sizeAsk] = "
		<< "[" << myMarket->getOrderBook(1)->getTotalBidQuantity() << " ; "
		<< myMarket->getOrderBook(1)->getTotalAskQuantity() << "]"
		<< std::endl ;
	std::cout << "Order book initialized." << std::endl ;
	plotOrderBook(myMarket, plotter, 1);

	// Plot the process of prices

	// Create one Noise Trader
	DistributionExponential* NoiseTraderActionTimeDistribution = new DistributionExponential(myRNG, meanActionTimeNT);
	DistributionUniform* NoiseTraderOrderTypeDistribution = new DistributionUniform(myRNG);
	//DistributionExponential * NoiseTraderOrderVolumeDistribution = new DistributionExponential(myRNG, meanVolumeNT) ;
	DistributionUniform* NoiseTraderOrderVolumeDistribution = new DistributionUniform(myRNG, minVolumeNT, maxVolumeNT);
	//DistributionConstant * NoiseTraderOrderVolumeDistribution = new DistributionConstant(myRNG, meanVolumeNT) ;
	NoiseTrader* myNoiseTrader = new NoiseTrader(myMarket,
	                                             NoiseTraderActionTimeDistribution,
	                                             NoiseTraderOrderTypeDistribution,
	                                             NoiseTraderOrderVolumeDistribution,
	                                             buyFrequencyNT, 1);
	myMarket->registerAgent(myNoiseTrader);

	// Simulate market
	std::cout << "Simulation starts. " << std::endl ;
	double currentTime = simulationTimeStart;
	int i = 1;

	std::vector<double> MidPriceTimeseries;
	try
	{
		while (currentTime < simulationTimeStop)
		{
			//	std::cout<<"go while"<<std::endl;
			// Get next time of action
			currentTime += myMarket->getNextActionTime() ;
			// Select next player
			int spread = myMarket->getOrderBook(1)->getAskPrice() - myMarket->getOrderBook(1)->getBidPrice();
			Agent* actingAgent;

			if (spread >= 2 * myMarket->getOrderBook(1)->getTickSize())
			{
				actingAgent = myMarketMaker;
				actingAgent->makeAction(actingAgent->getTargetedStock(), currentTime) ;
			}

			else
			{
				actingAgent = myMarket->getNextActor() ;
			}

			if (actingAgent->getAgentType() == LIQUIDITY_PROVIDER)
			{
				int oldAskPrice = myMarket->getOrderBook(1)->getAskPrice();
				int oldBidPrice = myMarket->getOrderBook(1)->getBidPrice();
				myMarket->getOrderBook(1)->cleanOrderBook();
				myLiquidityProvider->cleanPending();
				myMarketMaker->cleanPending();
				myMarket->getOrderBook(1)->setDefaultBidAsk(oldBidPrice, oldAskPrice);
				myLiquidityProvider->makeAction(actingAgent->getTargetedStock(), currentTime, true);
			}
			else
			{
				// Submit order
				if (actingAgent->getAgentType() == NOISE_TRADER)
				{
					actingAgent->makeAction(actingAgent->getTargetedStock(), currentTime) ;
				}
			}


			// From time to time, check state of order book
			if (currentTime > i * printIntervals)
			{
				std::cout
					<< "time: " << currentTime << std::endl
					<< "[bid;ask]="
					<< "[" << myMarket->getOrderBook(1)->getBidPrice() / 100.0 << " ; "
					<< myMarket->getOrderBook(1)->getAskPrice() / 100.0 << "]" << std::endl
					<< "[sizeBid;sizeAsk]="
					<< "[" << myMarket->getOrderBook(1)->getTotalBidQuantity() << ";" <<
					+ myMarket->getOrderBook(1)->getTotalAskQuantity() << "]"
					<< std::endl
					<< "[pendingLP ; pendingNT ; pendingMM]="
					<< "[" << myLiquidityProvider->getPendingOrders()->size() << ";"
					<< myNoiseTrader->getPendingOrders()->size() << ";"
					<< myMarketMaker->getPendingOrders()->size() << "]"
					<< std::endl;
				// Plot order book

				plotOrderBook(myMarket, plotter, 1);


				// Agents'portfolios
				std::cout << "LP: nStock=\t" << myLiquidityProvider->getStockQuantity(1)
					<< "\t Cash=\t" << myLiquidityProvider->getNetCashPosition() << std::endl ;
				std::cout << "NT: nStock=\t" << myNoiseTrader->getStockQuantity(1)
					<< "\t Cash=\t" << myNoiseTrader->getNetCashPosition() << std::endl;
				std::cout << "MM: nStock=\t" << myMarketMaker->getStockQuantity(1)
					<< "\t Cash=\t" << myMarketMaker->getNetCashPosition() << std::endl << std::endl << std::endl ;
				// Update sampling

				i++;
			}
			// Update clock
			myMarket->setNextActionTime() ;
		}
	}
	catch (Exception& e)
	{
		std::cout << e.what() << std::endl ;
	}
	int nbLPStocks = myLiquidityProvider->getStockQuantity(1);
	int nbMMStocks = myMarketMaker->getStockQuantity(1);
	int cashLP = myLiquidityProvider->getNetCashPosition();
	int cashMM = myMarketMaker->getNetCashPosition();
	if (nbLPStocks > 0)
	{
		cashLP += nbLPStocks * myMarket->getOrderBook(1)->getAskPrice();
	}
	else if (nbLPStocks < 0)
	{
		cashLP += nbLPStocks * myMarket->getOrderBook(1)->getBidPrice();
	}
	if (nbMMStocks > 0)
	{
		cashMM += nbMMStocks * myMarket->getOrderBook(1)->getAskPrice();
	}
	else if (nbMMStocks < 0)
	{
		cashMM += nbMMStocks * myMarket->getOrderBook(1)->getBidPrice();
	}
	std::cout << "CASH POSITIONS : " << std::endl;
	std::cout << "LP: CASH =\t" << cashLP / 100.0 << std::endl;
	std::cout << "MM: CASH =\t" << cashMM / 100.0 << std::endl;
	std::cout << "NT: CASH =\t" << myNoiseTrader->getNetCashPosition() / 100.0 << std::endl;

	historicPrices = myMarket->getOrderBook(1)->getHistoricPrices();
	transactionsTimes = myMarket->getOrderBook(1)->getTransactionsTimes();
	plotter2->plotPrices(transactionsTimes, historicPrices);
	std::cout << "passé!!!" << std::endl;

	delete myMarket;
	delete plotter;
	delete plotter2;
	delete myRNG;
	delete MarketOrderActionTimeDistribution;
	delete MarketOrderOrderPriceDistribution;
	delete MarketOrderOrderVolumeDistribution;
	delete myMarketMaker;
	delete NoiseTraderActionTimeDistribution;
	delete NoiseTraderOrderTypeDistribution;
	delete NoiseTraderOrderVolumeDistribution;
	delete myNoiseTrader;

	int a;
	std::cin >> a;

	// Print stored history
	// myMarket->getOrderBook(1)->printStoredOrderBookHistory();


	/*	// Stats on mid price
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
	*/
	return 0;
}
