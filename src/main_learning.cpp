	
#include "LearningGame.h"


int main(int argc, char* argv[])
{
	
	int nbr_episodes = 100;
	int nbr_probs = 21;
	double min_prob = 0;
	double max_prob = 1;
	if (argc > 1) {
		nbr_episodes = std::stoul(argv[1]);
	}
	if (argc > 2) {
		nbr_probs = std::stoul(argv[2]);
	}
	if (argc > 3){
		min_prob = std::stod(argv[3]);
	}
	if (argc > 4){
		max_prob = std::stod(argv[4]);
	}

	LearningGame game{nbr_probs, min_prob, max_prob};

	game.play(nbr_episodes);


}