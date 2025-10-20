#include <iostream>

// Function declarations for each test
void Card_test();
void CardStack_test();
void Deck_test();
void Hand_test();
void History_test();
// void InteractiveAgent_test();
// void InteractiveGame_test();
// void LearningGame_test();
// void SoundAgent_test();
void State_test();
void utils_test();

int main() {
    
#ifdef DEBUG
	utils_test();
	Card_test();
	CardStack_test();
	Hand_test();
	Deck_test();
	State_test();
	History_test();
#endif
    std::cout << "Running all tests..." << std::endl;
    Card_test();
    CardStack_test();
    Deck_test();
    Hand_test();
    History_test();
    // InteractiveAgent_test();
    // InteractiveGame_test();
    // LearningGame_test();
    // SoundAgent_test();
    State_test();
    utils_test();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}