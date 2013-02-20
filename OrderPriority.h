#include "Order.h"

class OrderPriority
{
public:

	bool operator()(const Order& u, const Order& v) const {
		return u.getPriority() > v.getPriority();
	}
};
