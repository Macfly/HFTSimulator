#include "Exceptions.h"
#include "Types.h"
#include "Market.h"
#include "OrderBook.h"
#include "Order.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>

OrderBook::OrderBook(Market *a_market, int a_identifier, int a_tickSize, int a_defaultBid, int a_defaultAsk)
{
	m_identifier = a_identifier;
	m_linkToMarket	= a_market;
	m_tickSize = a_tickSize;
	m_defaultBid = a_defaultBid;
	m_defaultAsk = a_defaultAsk;
	m_last = 0;
	m_lastQ= 0;
	m_storeOrderBookHistory = false;
	m_storeOrderHistory = false;
	m_printHistoryonTheFly = false;
	m_maxDepth = 4;
	m_headerPrinted = false;
	m_returnsSumSquared = 0.0;
	m_historicPrices.push_back(10000); 
	m_transactionsTimes.push_back(0);
}
OrderBook::~OrderBook()
{

}
Market * OrderBook::getLinkToMarket()
{
	return m_linkToMarket ;
}
void OrderBook::setStoreOrderBookHistory(bool a_store, int a_depth)
{
	m_storeOrderBookHistory = a_store;
	m_maxDepth = a_depth;
}
void OrderBook::setPrintOrderBookHistory(bool a_print, int a_depth)
{
	m_printHistoryonTheFly = a_print;
	m_maxDepth = a_depth;
}
void OrderBook::setStoreOrderHistory(bool a_store)
{
	m_storeOrderHistory = a_store;
}

int OrderBook::getIdentifier() const
{
	return m_identifier ;
}
int OrderBook::getAskPrice() const
{
	if (m_asks.empty())
		return m_defaultAsk;
	else
		return m_asks.begin()->first;
}
int OrderBook::getBidPrice() const
{
	if (m_bids.empty())
		return m_defaultBid;
	else
		return m_bids.rbegin()->first;
}
int OrderBook::getTickSize() const
{
	return m_tickSize;
}
int OrderBook::getBidQuantity() const
{
	int l_quantity,l_bid;
	concurrency::concurrent_unordered_map< int , int>::const_iterator it = bids_quantity.begin();
	l_bid = (*it).first;
	l_quantity = getBidQuantityForThisPrice(l_bid);
	return l_quantity;
}
int OrderBook::getAskQuantity() const
{
	int l_quantity,l_ask;
	concurrency::concurrent_unordered_map< int , int>::const_iterator it = asks_quantity.begin();
	l_ask = (*it).first;
	l_quantity = getAskQuantityForThisPrice(l_ask);
	return l_quantity;
}
int OrderBook::getDistanceToBestOppositeQuote(int a_price) const
{
	int l_ask = getAskPrice();
	int l_bid = getBidPrice();
	int distance;
	if	(a_price>=l_ask)
	{
		distance = a_price - l_bid;
	}
	else
	{
		distance = l_ask - a_price;
	}
	return (distance/m_tickSize);
}
void OrderBook::runOrderBook()
{
	std::cout<<"thread started "<<std::endl;
	Order orderToExecute;
	while(open){
		if(orders.try_pop(orderToExecute)){
			switch(orderToExecute.m_type)
			{
			case LIMIT_SELL:
				std::cout << "limit sell" << std::endl;
				processLimitSellOrder(orderToExecute);
				break;
			case LIMIT_BUY:
				std::cout << "limit buy" << std::endl;
				processLimitBuyOrder(orderToExecute);
				break;
			case MARKET_SELL:
				std::cout << "market sell" << std::endl;
				processMarketSellOrder(orderToExecute);
				break;
			case MARKET_BUY:
				std::cout << "market buy" << std::endl;
				processMarketBuyOrder(orderToExecute);
				break;
			case CLEAR_OB:
				cleanOrderBook();				
				//setDefaultBidAsk(orderToExecute.m_newBid, orderToExecute.m_newAsk);
				break;
			default:
				break;
			} 
			//if(m_storeOrderHistory) m_orderHistory.push_back(orderToExecute) ;
			m_linkToMarket->updateCurrentTime(orderToExecute.m_time);
			m_linkToMarket->notifyAllAgents();
		}
	}
}
void OrderBook::processLimitBuyOrder(Order & a_order)
{
	// Check if this is not a crossing order
	int ask0 = getAskPrice() ;
	int bid0 = getBidPrice();
	if(ask0<=a_order.m_price)
	{
		processMarketBuyOrder(a_order);
	//	std::cout << "crossing limit order in OrderBook::processLimitBuyOrder. order.price = "<< a_order.m_price << 
		//	" ask0 : " << ask0 << " bid0 : " << bid0 << std::endl ;
	}
	else
	{
	// If OK, store order
	m_bids[a_order.m_price].push_back(a_order);

	bids_quantity[a_order.m_price] = get_value_map( bids_quantity, a_order.m_price, 0 ) + a_order.getVolume();
	// If this order is above bid[MAX_LEVEL], then it needs to be recorded in history
	//if(m_storeOrderBookHistory)
	//{
	//	int bidMAX = getBidPriceAtLevel(m_maxDepth) ;
	//	if(bidMAX<=a_order.m_price)
	//	{
	//		storeOrderBookHistory(a_order.m_time);
	//	}
	//}
	}
}
void OrderBook::processLimitSellOrder(Order & a_order)
{
	// Check if this is not a crossing order
	int bid0 = getBidPrice() ;
	int ask0 = getAskPrice() ;
	if(a_order.m_price<=bid0)
	{
		processMarketSellOrder(a_order);
		//std::cout << "crossing limit order in OrderBook::processLimitSellOrder. order.price = "<< a_order.m_price << 
			//" bid0 : " << bid0 << " ask0 : " << ask0 << std::endl ;
	}
	else
	{
	// If OK, store order
	m_asks[a_order.m_price].push_back(a_order);
	asks_quantity[a_order.m_price] = get_value_map( asks_quantity, a_order.m_price, 0 ) + a_order.getVolume();
	// If this order is above bid[MAX_LEVEL], then it needs to be recorded in history
	//if(m_storeOrderBookHistory)
	//{
	//	int askMAX = getAskPriceAtLevel(m_maxDepth) ;
	//	if(a_order.m_price<=askMAX)
	//	{
	//		storeOrderBookHistory(a_order.m_time);
	//	}
	//}
	}
}
void OrderBook::processMarketBuyOrder(Order & a_order)
{
	std::map<int,std::list < Order > > ::iterator iter;
	while(a_order.m_volume>0)
	{
		if(!m_asks.empty())
		{
			iter = m_asks.begin();
			Order *l_fifoOrder = &iter->second.front();
			if (l_fifoOrder->m_volume == a_order.m_volume)
			{
				try
				{
					m_linkToMarket->notifyExecution(l_fifoOrder->m_owner,l_fifoOrder->m_globalOrderIdentifier,a_order.m_time,l_fifoOrder->m_price);
					m_linkToMarket->notifyExecution(a_order.m_owner,a_order.m_globalOrderIdentifier,a_order.m_time,l_fifoOrder->m_price);
					a_order.m_volume = 0;
					m_last = l_fifoOrder->m_price;
					m_lastQ = l_fifoOrder->m_volume;
				}
				catch (int e)
				{
					std::cout << "An exception occurred. Exception Nr. " << e << std::endl;
				}
				/*if (m_storeOrderBookHistory)
				{
				storeOrderBookHistory(a_order.m_time);
				}
				if (m_printHistoryonTheFly)
				{
				printOrderBookHistoryOnTheFly(a_order.m_time);
				}*/
				iter->second.pop_front();
				asks_quantity[a_order.m_price] -= a_order.getVolume();
			}
			else if (l_fifoOrder->m_volume > a_order.m_volume)
			{
				try
				{
					m_linkToMarket->notifyPartialExecution(l_fifoOrder->m_owner,l_fifoOrder->m_globalOrderIdentifier,a_order.m_time,a_order.m_volume,l_fifoOrder->m_price);
					m_linkToMarket->notifyExecution(a_order.m_owner,a_order.m_globalOrderIdentifier,a_order.m_time,l_fifoOrder->m_price);
					l_fifoOrder->m_volume -= a_order.m_volume;
					asks_quantity[a_order.m_price] -= a_order.getVolume();
					a_order.m_volume = 0;
					m_last = l_fifoOrder->m_price;
					m_lastQ = l_fifoOrder->m_volume;
				}
				catch (int e)
				{
					std::cout << "An exception occurred. Exception Nr. " << e << std::endl;
				}
				/*if (m_storeOrderBookHistory)
				{
				storeOrderBookHistory(a_order.m_time);
				}
				if (m_printHistoryonTheFly)
				{
				printOrderBookHistoryOnTheFly(a_order.m_time);
				}*/
			}
			else if (l_fifoOrder->m_volume < a_order.m_volume)
			{
				try
				{
					m_linkToMarket->notifyExecution(l_fifoOrder->m_owner,l_fifoOrder->m_globalOrderIdentifier,a_order.m_time,l_fifoOrder->m_price);
					m_linkToMarket->notifyPartialExecution(a_order.m_owner,a_order.m_globalOrderIdentifier,a_order.m_time,l_fifoOrder->m_volume,l_fifoOrder->m_price);
					a_order.m_volume -= l_fifoOrder->m_volume;
					m_last = l_fifoOrder->m_price;
					m_lastQ = l_fifoOrder->m_volume;
				}
				catch (int e)
				{
					std::cout << "An exception occurred. Exception Nr. " << e << std::endl;
				}
				/*if (m_storeOrderBookHistory)
				{
				storeOrderBookHistory(a_order.m_time);
				}
				if (m_printHistoryonTheFly)
				{
				printOrderBookHistoryOnTheFly(a_order.m_time);
				}*/
				iter->second.pop_front();
				asks_quantity[a_order.m_price] -= a_order.getVolume();
			}
			if(iter->second.empty() && iter != m_asks.end()){
				try
				{
					m_asks.erase(m_asks.begin()) ;
				}
				catch (int e)
				{
					std::cout << "An exception occurred. Exception Nr. " << e << std::endl;
				}
			}
			m_historicPrices.push_back(m_last);
			m_transactionsTimes.push_back(m_linkToMarket->getCurrentTime());
			int sizePrices = m_historicPrices.size();
			double returns = double(double(double(m_historicPrices[sizePrices-1]) - double(m_historicPrices[sizePrices-2]))/double(m_historicPrices[sizePrices-2]));
			m_returnsSumSquared+=pow(returns,2);
		}
		else
		{
			std::ostringstream l_stream;
			l_stream<<"Not enough ask orders for asset "<<m_identifier;
			std::string l_string = l_stream.str();
			throw Exception(l_string.c_str());
		}
	}
}
void OrderBook::processMarketSellOrder(Order & a_order)
{
	std::map<int,std::list < Order > > ::reverse_iterator iter;
	while(a_order.m_volume>0)
	{
		if (!m_bids.empty())
		{
			iter = m_bids.rbegin();
			Order *l_fifoOrder = &iter->second.front();
			if (l_fifoOrder->m_volume == a_order.m_volume)
			{
				try
				{
					m_linkToMarket->notifyExecution(l_fifoOrder->m_owner,l_fifoOrder->m_globalOrderIdentifier,a_order.m_time,l_fifoOrder->m_price);
					m_linkToMarket->notifyExecution(a_order.m_owner,a_order.m_globalOrderIdentifier,a_order.m_time,l_fifoOrder->m_price);
					a_order.m_volume = 0;
					m_last = l_fifoOrder->m_price;
					m_lastQ = l_fifoOrder->m_volume;
				}
				catch (int e)
				{
					std::cout << "An exception occurred. Exception Nr. " << e << std::endl;
				}
				/*if (m_storeOrderBookHistory)
				{
				storeOrderBookHistory(a_order.m_time);
				}
				if (m_printHistoryonTheFly)
				{
				printOrderBookHistoryOnTheFly(a_order.m_time);
				}*/
				iter->second.pop_front();
				bids_quantity[a_order.m_price] -= a_order.getVolume();
			}
			else if (l_fifoOrder->m_volume > a_order.m_volume)
			{
				try
				{
					m_linkToMarket->notifyPartialExecution(l_fifoOrder->m_owner,l_fifoOrder->m_globalOrderIdentifier,a_order.m_time,a_order.m_volume,l_fifoOrder->m_price);
					m_linkToMarket->notifyExecution(a_order.m_owner,a_order.m_globalOrderIdentifier,a_order.m_time,l_fifoOrder->m_price);
					l_fifoOrder->m_volume -= a_order.m_volume;
					bids_quantity[a_order.m_price] -= a_order.getVolume();
					a_order.m_volume = 0;
					m_last = l_fifoOrder->m_price;
					m_lastQ = l_fifoOrder->m_volume;
				}
				catch (int e)
				{
					std::cout << "An exception occurred. Exception Nr. " << e << std::endl;
				}
				/*if (m_storeOrderBookHistory)
				{
				storeOrderBookHistory(a_order.m_time);
				}
				if (m_printHistoryonTheFly)
				{
				printOrderBookHistoryOnTheFly(a_order.m_time);
				}*/
			}
			else if (l_fifoOrder->m_volume < a_order.m_volume)
			{
				try
				{
					m_linkToMarket->notifyExecution(l_fifoOrder->m_owner,l_fifoOrder->m_globalOrderIdentifier,a_order.m_time,l_fifoOrder->m_price);
					m_linkToMarket->notifyPartialExecution(a_order.m_owner,a_order.m_globalOrderIdentifier,a_order.m_time,l_fifoOrder->m_volume,l_fifoOrder->m_price);
					a_order.m_volume -= l_fifoOrder->m_volume;
					bids_quantity[a_order.m_price] -= a_order.getVolume();
					m_last = l_fifoOrder->m_price;
					m_lastQ = l_fifoOrder->m_volume;
				}
				catch (int e)
				{
					std::cout << "An exception occurred. Exception Nr. " << e << std::endl;
				}
				/*if (m_storeOrderBookHistory)
				{
				storeOrderBookHistory(a_order.m_time);
				}
				if (m_printHistoryonTheFly)
				{
				printOrderBookHistoryOnTheFly(a_order.m_time);
				}*/
				iter->second.pop_front();
				bids_quantity[a_order.m_price] -= a_order.getVolume();
			}
			if(iter->second.empty() && iter != m_bids.rend()){
				try
				{
					m_bids.erase( --m_bids.end() ) ;
				}
				catch (int e)
				{
					std::cout << "An exception occurred. Exception Nr. " << e << std::endl;
				}
			}

			m_historicPrices.push_back(m_last);

			m_transactionsTimes.push_back(m_linkToMarket->getCurrentTime());
			int sizePrices = m_historicPrices.size();
			double returns = double(double(double(m_historicPrices[sizePrices-1]) - double(m_historicPrices[sizePrices-2]))/double(m_historicPrices[sizePrices-2]));
			m_returnsSumSquared+=pow(returns,2);

		}
		else
		{
			std::ostringstream l_stream;
			l_stream<<"Not enough ask orders for asset "<<m_identifier;
			std::string l_string = l_stream.str();
			throw Exception(l_string.c_str());
		}
	}
}
int OrderBook::getAskQuantityForThisPrice(int a_priceLevel) const
{
	return get_value_map( asks_quantity, a_priceLevel, 0 );
}

int OrderBook::getBidQuantityForThisPrice(int a_priceLevel) const
{
	return get_value_map( bids_quantity, a_priceLevel, 0 );
}

std::map< int , std::list < Order > > OrderBook::getBidQueue() const
{
	return m_bids;
}
std::map< int , std::list < Order > > OrderBook::getAskQueue() const
{
	return m_asks;
}

void OrderBook::processCancellation(int a_agentIdentifier,int a_orderIdentifier, double a_time)
{
	bool orderFoundinAsks = false ;
	bool orderFoundinBids = false ;
	std::map<int,std::list<Order> >::iterator itPrice ;
	std::list<Order>::iterator it_l_order ;
	// Look for order in m_asks
	try{
		itPrice = m_asks.begin();
		while(!orderFoundinAsks && itPrice != m_asks.end())
		{
			it_l_order = itPrice->second.begin() ;
			while (!orderFoundinAsks && it_l_order!=(*itPrice).second.end())
			{
				if(it_l_order->getIdentifier()==a_orderIdentifier){
					orderFoundinAsks = true ;
					break;
				}
				it_l_order++ ;
			}
			if(orderFoundinAsks) break ;
			itPrice++ ;
		}

	}
	catch (int e)
	{
		std::cout << "An exception occurred. Exception Nr. " << e << std::endl;
	}

	// If not found, look for order in m_bids
	if(!orderFoundinAsks)
	{
		try{
			itPrice = m_bids.begin();
			while(!orderFoundinBids && itPrice != m_bids.end())
			{
				it_l_order = itPrice->second.begin() ;
				while (!orderFoundinBids && it_l_order!=(*itPrice).second.end())
				{
					if(it_l_order->getIdentifier()==a_orderIdentifier){
						orderFoundinBids = true ;
						break;
					}
					it_l_order++ ;
				}
				if(orderFoundinBids) break ;
				itPrice++ ;
			}
		}
		catch (int e)
		{
			std::cout << "An exception occurred. Exception Nr. " << e << std::endl;
		}

	}
	if(orderFoundinBids || orderFoundinAsks)
	{
		// check owner
		if(it_l_order->getOwner()!=a_agentIdentifier){
			std::cout << "Owner mismatch in OrderBook::processCancellation." << std::endl ;
			exit(1) ;
		}
		// erase it
		itPrice->second.erase(it_l_order) ;
		//pop the queue from the list if empty
		try{
			if(itPrice->second.empty())
			{
				if(orderFoundinBids)
				{
					m_bids.erase(itPrice);
				}
				else
				{
					m_asks.erase(itPrice);
				}
			}
		}
		catch (int e)
		{
			std::cout << "An exception occurred. Exception Nr. " << e << std::endl;
		}

		// notify owner of order
		m_linkToMarket->notifyCancellation(a_agentIdentifier,a_orderIdentifier,a_time) ;
		m_linkToMarket->updateCurrentTime(a_time);
		m_linkToMarket->notifyAllAgents();
	}
	// ... else raise error
	else{
		std::cout << "Order not found in OrderBook::processCancellation." << std::endl ;
		exit(1) ;
	}
}

const std::vector<OrderBookHistory> & OrderBook::getOrderBookHistory() const
{
	return m_orderBookHistory;
}

const std::vector<Order> & OrderBook::getOrderHistory() const
{
	return m_orderHistory;
}

int OrderBook::getTotalBidQuantity()
{
	int totalBidQuantity = 0 ;
	concurrency::concurrent_unordered_map<int, int>::iterator it = bids_quantity.begin() ;

	while(it != bids_quantity.end()){
		totalBidQuantity += it->first;
		it++ ;
	}
	return totalBidQuantity ;
}

int OrderBook::getTotalAskQuantity()
{
	int totalAskQuantity = 0 ;
	concurrency::concurrent_unordered_map<int, int>::iterator it = asks_quantity.begin() ;
	while(it != asks_quantity.end()){
		totalAskQuantity += it->second;
		it++ ;
	}
	return totalAskQuantity ;
}


void OrderBook::cleanOrderBook(){

	m_asks.clear();
	m_bids.clear();

	asks_quantity.clear();
	bids_quantity.clear();


}

void OrderBook::setDefaultBidAsk(int bid, int ask){
	m_defaultBid = bid;
	m_defaultAsk = ask;
}

std::vector<int> OrderBook::getHistoricPrices(){
	return m_historicPrices;
}

std::vector<double> OrderBook::getTransactionsTimes(){
	return m_transactionsTimes;
}

double OrderBook::getReturnsSumSquared(){
	return m_returnsSumSquared;
}

void OrderBook::pushOrder(Order &order){
	orders.push(order);
}

void OrderBook::closeOrderBook(){
	open = false;
}

int OrderBook::getPrice() const
{
	return m_last;
}

void OrderBook::getOrderBookForPlot(std::vector<int> &a_price,std::vector<int> &a_priceQ) const
{

	std::map< int , std::list < Order > >::const_iterator itBids; 
	std::map< int , std::list < Order > >::const_iterator itAsks; 

	std::vector<LimitOrders> l_vector;
	for(itBids = m_bids.begin();itBids != m_bids.end();itBids++)
	{
		int l_bid = (*itBids).first;
		a_price.push_back(l_bid);
		a_priceQ.push_back(-1*getBidQuantityForThisPrice(l_bid));
	}
	for(itAsks = m_asks.begin();itAsks != m_asks.end();itAsks++)
	{
		int l_ask = (*itAsks).first;
		a_price.push_back(l_ask);
		a_priceQ.push_back(getAskQuantityForThisPrice(l_ask));
	}


}