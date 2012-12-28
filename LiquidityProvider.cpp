#include "Market.h"
#include "OrderBook.h"
#include "Order.h"
#include "Distribution.h"
#include "DistributionUniform.h"
#include "LiquidityProvider.h"
#include "LiquidityProvider.h"
#include <cmath>
#include <iostream>

LiquidityProvider::LiquidityProvider(
		Market * a_market, 
		Distribution * a_ActionTimeDistribution,
		Distribution * a_OrderVolumeDistribution,
		Distribution * a_OrderPriceDistribution,
		double a_buyFrequency,
		int a_favouriteStockIdentifier,
		double a_cancelBuyFrequency,
		double a_cancelSellFrequency,
		double a_cancelProbability
		):Agent(a_market,LIQUIDITY_PROVIDER, a_favouriteStockIdentifier)
{
	m_ActionTimeDistribution = a_ActionTimeDistribution ;
	m_OrderVolumeDistribution = a_OrderVolumeDistribution ;
	m_OrderPriceDistribution = a_OrderPriceDistribution;
	m_buyFrequency = a_buyFrequency ;
	m_OrderTypeDistribution = new DistributionUniform(a_market->getRNG()) ;
	m_cancelBuyFrequency = a_cancelBuyFrequency;
	m_cancelSellFrequency = a_cancelSellFrequency;
	m_sellFrequency = 1.0 - m_cancelBuyFrequency - m_cancelSellFrequency - m_buyFrequency;
	m_cancelProbability = a_cancelProbability ;
	m_cancelDistribution = new DistributionUniform(a_market->getRNG()) ;

}
LiquidityProvider::~LiquidityProvider() 
{

}
double LiquidityProvider::getNextActionTime() const
{
	return m_ActionTimeDistribution->nextRandom() ;
}
int LiquidityProvider::getOrderVolume() const
{
	return std::max((int)m_OrderVolumeDistribution->nextRandom(),1) ;		
}

int LiquidityProvider::getOrderPrice(int a_OrderBookId, OrderType a_OrderType) const
{
	int price ;
	double lag = m_OrderPriceDistribution->nextRandom() ;
	int tickSize = m_linkToMarket->getOrderBook(a_OrderBookId)->getTickSize() ;
	if(a_OrderType==LIMIT_BUY)
	{
		int currentPrice = m_linkToMarket->getOrderBook(a_OrderBookId)->getAskPrice() ;
		price = currentPrice - (((int)lag)*tickSize) - tickSize ;
	}
	else if(a_OrderType==LIMIT_SELL)
	{
		int currentPrice = m_linkToMarket->getOrderBook(a_OrderBookId)->getBidPrice() ;
		price = currentPrice + (((int)lag)*tickSize) + tickSize ;
	}
	else
	{
		// GESTION DES ERREURS ET EXCEPTIONS	
	}
	return price ;
}
OrderType LiquidityProvider::getOrderType() const
{
	double l_orderTypeAlea = m_OrderTypeDistribution->nextRandom();
	if(l_orderTypeAlea<m_cancelBuyFrequency)
	{
//		std::cout << "CANCEL_BUY" << std::endl ;
		return CANCEL_BUY;
	}
	else if(l_orderTypeAlea<m_cancelBuyFrequency + m_cancelSellFrequency)
	{
//		std::cout << "CANCEL_SELL" << std::endl ;
		return CANCEL_SELL;
	}
	else if(l_orderTypeAlea<m_cancelBuyFrequency + m_cancelSellFrequency + m_buyFrequency)
	{
//		std::cout << "LIMIT_BUY" << std::endl ;
		return LIMIT_BUY ;
	}
	else
	{
//		std::cout << "LIMIT_SELL " << std::endl ;
		return LIMIT_SELL ;
	}
}

void LiquidityProvider::makeAction(int a_OrderBookId, double a_currentTime)
{
		OrderType thisOrderType = getOrderType() ;
		if(thisOrderType == CANCEL_BUY ||thisOrderType == CANCEL_SELL)
		{
			if (a_currentTime == 0.0)// NO Cancellation at the initialisation of the process
			{
				return;
			}
			else
			{
				if(thisOrderType == CANCEL_BUY)
				{
					chooseOrdersToBeCanceled(a_OrderBookId,true,a_currentTime);
					return;
				}
				else
				{
					chooseOrdersToBeCanceled(a_OrderBookId,false,a_currentTime);
					return;
				}
			}
		}
		int thisOrderVolume = getOrderVolume() ;
		int thisOrderPrice = getOrderPrice(a_OrderBookId, thisOrderType) ;
		submitOrder(
			a_OrderBookId, a_currentTime,
			thisOrderVolume,
			thisOrderType,
			thisOrderPrice
		);	
}

void LiquidityProvider::chooseOrdersToBeCanceled(int a_OrderBookId, bool a_buySide, double a_time)
{
	std::map<int,Order> pendingOrdersCopy(m_pendingOrders) ;
	std::map<int,Order>::iterator iter = pendingOrdersCopy.begin();

	while(iter!=pendingOrdersCopy.end()){
		OrderType thisOrderType = iter->second.getType() ;
		if((thisOrderType==LIMIT_BUY && a_buySide)||(thisOrderType==LIMIT_SELL && !a_buySide))
		{
			double cancelAlea = m_cancelDistribution->nextRandom() ;
			if(cancelAlea<m_cancelProbability){
				submitCancellation(a_OrderBookId,iter->second.getIdentifier(),a_time) ;
			}
		}
		iter++ ;
	}
}
void LiquidityProvider::processInformation()
{
	// For exemple, read the market book history and decide to do something within a reaction time
	
}

