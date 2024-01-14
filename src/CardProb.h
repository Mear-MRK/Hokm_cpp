
#pragma once

#include "Card.h"

struct CardProb {
public:
	float prob = 0;
	Card card = Card::NONE;

	bool operator>(const CardProb& rhs) const{
		return prob > rhs.prob;
	}
	bool operator<(const CardProb& rhs) const{
		return prob < rhs.prob;
	}
	bool operator>=(const CardProb& rhs) const{
		return prob >= rhs.prob;
	}
	bool operator<=(const CardProb& rhs) const{
		return prob <= rhs.prob;
	}
//	bool operator==(const CardProb& rhs) const{
//		return prob == rhs.prob;
//	}

};
