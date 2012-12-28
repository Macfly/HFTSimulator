#include <fstream>
#include <iostream>
#include <cstdio>
#include "Plot.h"


Plot::Plot()
{
#ifdef WIN32
	m_gnuPlot = _popen("GnuPlot\\pgnuplot", "w");
#else
	m_gnuPlot = popen("gnuplot", "w");
#endif
}

Plot::~Plot()
{
	
}
void Plot::plot()
{
	//! on crit les donnes dans un fichier
	std::ofstream outFile("graphe.data");
	for(int i=0;i<100;i++)
	{
		outFile<<(double)i<<'\t'<<(double)i<<'\n';
	}
	outFile.close();
	
	//! on lance la commande plot
	fflush (m_gnuPlot);
	fprintf(m_gnuPlot, "plot \"graphe.data\" with lines\n");
	fflush (m_gnuPlot);

}
//-----------------------------------------------------------------------------
void Plot::plot(std::string dataName, int size, const double *x, const double *y)
{
	//! on ecrit les donnes dans un fichier
	std::ofstream outFile("graphe.data");
	for(int i=0;i<size;i++)
	{
		outFile<<x[i]<<'\t'<<y[i]<<'\n';
	}
	outFile.close();
	
	//! on lance la commande plot
	fflush (m_gnuPlot);
	std::ostringstream oss_command ;
	oss_command << "plot \'graphe.data\' title \' " << dataName << "\' with lines\n" ;
	fprintf(m_gnuPlot, "%s", oss_command.str().c_str());
	fflush (m_gnuPlot);

}
//-----------------------------------------------------------------------------
void Plot::plotOrderBook(	const std::vector<int> & x, 
				   const std::vector<int> & y,int last)
{
	std::ofstream outFile("OrderBook.data");
	for(unsigned int i=0;i<x.size();i++)
	{
		outFile<<x[i]/100.0<<'\t'<<y[i]<<'\n';
	}
	outFile.close();
	
	//! on lance la commande plot
	fflush (m_gnuPlot);
	fprintf(m_gnuPlot, "set title \"Last Price = %f\"\n",  (double)last/100.0);
	fprintf(m_gnuPlot, "plot \"OrderBook.data\" with boxes\n");
	fflush (m_gnuPlot);

}
void Plot::plot2OrderBooks(const std::vector<int> & x1, const std::vector<int> & y1,int last1,
							const std::vector<int> & x2, const std::vector<int> & y2,int last2)


{
	std::ofstream outFile1("OrderBook1.data");
	for(unsigned int i=0;i<x1.size();i++)
	{
		outFile1<<x1[i]/100.0<<'\t'<<y1[i]<<'\n';
	}
	outFile1.close();

	
	std::ofstream outFile2("OrderBook2.data");
	for(unsigned int i=0;i<x2.size();i++)
	{
		outFile2<<x2[i]/100.0<<'\t'<<y2[i]<<'\n';
	}
	outFile2.close();

	//! on lance la commande plot
	fflush (m_gnuPlot);
	fprintf(m_gnuPlot, "set size 1.0, 1.0\n");
	fprintf(m_gnuPlot, "set origin 0.0, 0.0\n");
	fprintf(m_gnuPlot, "set multiplot\n");
	fprintf(m_gnuPlot, "set size 1.0,0.5\n");
	fprintf(m_gnuPlot, "set origin 0.0, 0.5\n");
	fprintf(m_gnuPlot, "set title \"Last Price = %f\"\n",  (double)last1/100.0);
	fprintf(m_gnuPlot, "plot \"OrderBook1.data\" with boxes\n");
	fprintf(m_gnuPlot, "set size 1.0,0.5\n");
	fprintf(m_gnuPlot, "set origin 0.0, 0.0\n");
	fprintf(m_gnuPlot, "set title \"Last Price = %f\"\n",  (double)last2/100.0);
	fprintf(m_gnuPlot, "plot \"OrderBook2.data\" with boxes\n");
	fflush (m_gnuPlot);
}




