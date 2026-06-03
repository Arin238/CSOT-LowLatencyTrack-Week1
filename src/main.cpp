#include <iostream>
#include "../include/engine.hpp"

int main() {
	Engine engine;

	// Load a small sample dataset shipped with the repo.
	engine.load_ticks("data/synthetic_small.csv");

	// Create the strategy from the factory symbol in the ABI header.
	csot::Strategy* strat = create_strategy();
	if (!strat) {
		std::cerr << "Failed to create strategy\n";
		return 1;
	}

	engine.run(*strat);

	delete strat;
	return 0;
}