# Hokm Card Game Project

This project is an implementation of the popular Iranian card game, Hokm, using C++ and Python. It provides a framework for simulating Hokm games, developing and testing different AI agents, and playing interactively with human players.

## Project Structure

The project is organized into several C++ classes and a Python client for remote interaction:

**C++ Classes:**

*   `Agent`: Base class for all Hokm agents (AI or human).
*   `RndAgent`: A simple random agent that plays randomly.
*   `SoundAgent`: A more sophisticated AI agent that uses heuristics and probability calculations to make decisions.
*   `InteractiveAgent`: Allows human players to interact with the game through a console interface.
*   `RemoteInterAgent`: Extends `InteractiveAgent` to enable remote interaction via a TCP socket connection.
*   `Card`: Represents a single playing card with suit and rank.
*   `CardStack`: A collection of cards, used for decks, hands, and played cards.
*   `Deck`: Represents a standard 52-card deck and provides shuffling and dealing functionalities.
*   `Hand`: Represents a player's hand of cards.
*   `History`: Stores the history of played cards in a round.
*   `State`: Represents the current state of a trick (cards played, turn, etc.).
*   `GameRound`: Manages a single round of Hokm, including trump calling, dealing, trick-taking, and scoring.
*   `InteractiveGame`: Facilitates interactive Hokm games with human players, either locally or remotely.
*   `LearningGame`: A framework for training and evaluating AI agents by playing multiple rounds against each other.

**Python Client:**

*   **`hokm_client`**: The main entry point for the Python client.
*   **`hokm/client.py`**: Provides a `HokmClient` class that manages the connection to the server and the `curses`-based terminal interface.

## How to Use

1.  **Compilation (C++):** Compile the C++ code using the provided `Makefile`. Simply run `make` in the root of the project. This will create the `hokm.out` and `hokm_dbg.out` executables.
2.  **Running the Server (C++):** Execute the compiled C++ executable (e.g., `./hokm.out`). It will start a Hokm server listening for client connections.
3.  **Running the Client (Python):** Run the `hokm_client` script, providing the server's IP address (or 'localhost' if running on the same machine) and the player ID as command-line arguments (e.g., `./hokm_client localhost 1`).
4.  **Interactive Play:** The Python client will display the game interface in the terminal, allowing you to interact with the game.

## Key Features

*   **AI vs. AI:** Simulate Hokm games between different AI agents to evaluate their performance.
*   **Human vs. AI:** Play Hokm against the AI agents to test your skills.
*   **Remote Play:** Play Hokm remotely with other human players through the Python client.
*   **Learning Framework:** The `LearningGame` class provides a foundation for developing and training AI agents using reinforcement learning or other techniques.

## Future Enhancements

*   **GUI:** Develop a graphical user interface (GUI) for a more visually appealing and user-friendly experience.
*   **Advanced AI:** Implement more advanced AI agents using machine learning algorithms.
*   **Online Multiplayer:** Extend the remote play functionality to support online multiplayer games.
*   **Tournament Mode:** Create a tournament mode for AI agents to compete against each other.

Feel free to contribute to this project by adding new features, improving the AI agents, or fixing bugs. Let's make this Hokm implementation even better!