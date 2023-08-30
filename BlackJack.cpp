#include "mbed.h"
#include <cstdio>
#include "stdlib.h"

// This is the Game object which contains functions related to rendering and betting, 
// as well as win conditions and the checks that produce those conditions. This object
// also holds win and loss data and the sum of money the player has.
struct Game{
    // win conditions, checks and data related to that
	bool checks(int dealerSum, int playerSum){
		if((dealerSum == playerSum) || (dealerSum > 21 && playerSum > 21)){
			draw();
			return true;
		}
		else if((playerSum > dealerSum || dealerSum > 21) && playerSum <= 21){
			win();
			return true;
		}
		else if((dealerSum > playerSum || playerSum > 21) && dealerSum <= 21){
			lose();
			return true;
		}
		else{ 
			return false;
		}
	}
    // This functions are seperated from the Checks function because they may need 
    // to be called on their own in the main game loop.
	void draw(){
		printf("draw\n");
		gamesTotal++;
        printBallance();
        bet = 0;
	}
	void win(){
		printf("win\n");
		winTotal++;
		gamesTotal++;
        playerBallance = playerBallance + bet;
        printBallance();
        bet = 0;
	}
	void lose(){
		printf("lose\n");
		gamesTotal++;
        playerBallance = playerBallance - bet;
        printBallance();
        bet = 0;
	}
    int wins(){
        return winTotal;
    }
    int games(){
        return gamesTotal;
    }
    // betting and money
    void incrementBet(){
        if(bet < playerBallance){
            bet = bet + 100;
            printf("\033[2K\rYour bet: £%d", bet);
        }
        else{
            printf("\033[2K\rYou cannot bet that high.");
            thread_sleep_for(600);
            printf("\033[2K\rYour bet: £%d", bet);
        }
    }
    void decrementBet(){
        if(bet > 0){
            bet = bet - 100;
            printf("\033[2K\rYour bet: £%d", bet);
        }
        else{
            printf("\033[2K\rYou cannot bet that low.");
            thread_sleep_for(600);
            printf("\033[2K\rYour bet: £%d", bet);
        }
    }
    int returnBallance(){
        return playerBallance;
    }
    // rendering
    void printBallance(){
        printf("Your ballance: £%d\n", playerBallance);
    }
    int printCard(int card, bool player){
        if(player){
            printf("Player");
        }
        else{
            printf("Dealer");
        }
        const char valueDecoder[] = "A234567890JQK";
        const char suitDecoder[] = "HDCS";
        char suit = suitDecoder[card%4];
        char value = valueDecoder[card%13];
        if(value == '0'){
            printf(": 10:%c\n", suit);
        }
        else{
            printf(": %c:%c\n", value, suit);
        }
        // the card is returned because this function will be nested between two other functions
        // when its called in the main game loop. This basically just makes the code look nicer.
        return card; 
    }
	private:
		int winTotal = 0;
		int gamesTotal = 0;
        int playerBallance = 1000;
        int bet = 0;
};

// The Deck object is used to generate, shuffle and return cards from the deck.
// The deck itself is moddled as an array of 52 numbers ranging from 0 to 51.
struct Deck{
	int topCard(){
		int card = deck[cardCounter];
		cardCounter++;
		if(cardCounter == 52) shuffle(); // checks if we are at the end of the deck
		return card;
	}
	void shuffle(){
		cardCounter = 0;
        // This loop sets all the numbers in the deck array to 100 
        //(It could be any number as long as it is not between 0 and 51)
		for(int i=0; i<52; i++){ 
			deck[i] = 100;
		}
		srand(SEED);
        bool noCardFound = true; 
		int position;
        // The loop will increment through each posible card and then randomly 
        // select a position in the deck for it using the SEED variable. If that
        // position has already been occupied it will generate a new one.
		for(int i=0; i<52; i++){
			while(noCardFound){
				position = rand() % 52;
				if(deck[position] == 100){
					deck[position] = i;
					noCardFound = false;
				}
			}
            noCardFound = true;
		}
	}
    void updateSeed(int NEWSEED){
        SEED = NEWSEED;
    }
	private:
            int SEED;
            int deck[52];
            int cardCounter = 0; // this card counter variable keeps track of how many cards have been dealt.
};

// The Cards object is used to manage the cards of both the dealer and the player.
// It only has one input function (getCard) and one output function (cardSum).
struct Cards{
    void getCard(int card){
        if(sum < 21){ // stops them taking cards if they are bust or on 21 
            for(int i=0; i<11; i++){
                if(cards[i] == 0){
                    cards[i] = valueDecoder[card%13]; // adds the value of the card to the cards array
                    sumValues();
                    return;
                }
            }
        }
    }
	int cardSum(){
		return sum;
	}
	private:
		const int valueDecoder[13] = {11,2,3,4,5,6,7,8,9,10,10,10,10};
        // The cards array stores the values of the cards in the players hand, the length is 11
        // because that is the max number of cards a player can have without ending the game.
        int cards[11] = {0}; 
		int sum = 0;

		void sumValues(){
		    bool aceCheck = false;
			while(true){
				aceCheck = true;
				sum = 0;
				for(int i=0; i<11; i++){
					sum = sum + cards[i];
				}
                // This loop exists to check if there are any aces in the players hand. If there are aces 
                // and the players sum is greater than 21 then they have to be reduced to a value of 1.
                // This has to be done seperatly for every ace found in the players hand, thats why the 
                // boolian variable aceCheck is used.
				if(sum > 21){ 
					for(int i=0; i<11; i++){
						if(cards[i] == 11 && aceCheck){
							cards[i] = 1;
							aceCheck = false;
                            // at this point if an ace is found it will leave the for loop and loop back up
                            // to the origional summing loop.
						}
					}
					if(sum > 21 && aceCheck){ // if no ace is found
						return;
					}
				}
				else return; // if the sum is not > 21
			}
		}
};

// This function is used to avoid duplicated code in the main game loop. 
// It adds cards to the dealer until it reaches or excedes 16. 
void dealersTurn(Game *game, Deck *deck, Cards *player, Cards *dealer){
    bool turnActive = true;
	while(turnActive){
		if(dealer->cardSum() < 16 && dealer->cardSum() <= player->cardSum()){
            dealer->getCard(game->printCard(deck->topCard(), false));
		}
		else if(game->checks(dealer->cardSum(), player->cardSum())){
			turnActive = false;
		}
	}	
}

int main(){
    // Inputs
    AnalogIn SEED(A0); // the seed is created by measuring the value on an analogue pin that has been left floating.
    DigitalIn HIT(D4, PullDown);
    DigitalIn HOLD(D5, PullDown);
    DigitalIn BET(D6, PullDown);
    // game variables
	bool gameActive = true;
    int dealerCard;
    // Initialisation
	Game game;
	Deck deck;
    // generates and shuffles a new deck
    // The SEED value is multiplied by 10000 and cast as an integer because its origionally a float and wont be suitable as a seed
    deck.updateSeed((int)(SEED*10000));
    deck.shuffle();
	while(true){ // main game loop
        // These objects will be reinitialised every time a new hand is started
		Cards player;
		Cards dealer;
        deck.updateSeed((int)(SEED*10000)); 
        game.printBallance();
        // Checks if you are out of money
        if(game.returnBallance() <= 0){
            printf("You are out of money, \nplease leave through the lobby.\n");
            while(true);
        }
        // Betting
        printf("Place your bet now.\nYour bet: £0");
        while(!HIT){
            if(BET){
                game.decrementBet();
                thread_sleep_for(500);
            }
            else if(HOLD){
                game.incrementBet();
                thread_sleep_for(500);
            }
        }
        thread_sleep_for(500);
        printf("\nBet locked in.\n");
        // Dealing the cards
        // These functions are nested together to prevent this code from being very long for no reason.
        // it starts off by getting a card from the deck, it then prints that card and tell the printCard() 
        // function whether its the dealer or a player with the bool. After that it passes that card to either
        // the player or the dealer.
        player.getCard(game.printCard(deck.topCard(), true));
        dealer.getCard(game.printCard(deck.topCard(), false));
        player.getCard(game.printCard(deck.topCard(), true));
        dealerCard = deck.topCard(); // the dealers second card has to be taken now but cannot be shown to the player
		dealer.getCard(dealerCard);
        // checks for an immediate blackjack
		if(player.cardSum() == 21){
            printf("BLACKJACK!\n");
			if(dealer.cardSum() == 21){
				game.draw();
			}
			else{ // The dealer will try to catch up if he doesnt also have a blackjack
                game.printCard(dealerCard, false);
                dealersTurn(&game, &deck, &player, &dealer);
			}
		}
		else{
			while(gameActive){ // this loop is used for the players input
				if(HIT){
                    printf("Hit\n");
                    player.getCard(game.printCard(deck.topCard(), true)); // player gets another card
					if(player.cardSum() > 21){ // the player has went over the max value of 21
                        printf("Bust\n");
                        game.printCard(dealerCard, false);
						game.lose();
						gameActive = false;
					}
					else if(player.cardSum() == 21){
                        game.printCard(dealerCard, false);
                        dealersTurn(&game, &deck, &player, &dealer); // dealer tries to catch up
                        gameActive = false;
					}
                    thread_sleep_for(500);
				}
				else if(HOLD){
                    printf("Hold\n");
                    game.printCard(dealerCard, false);
                    dealersTurn(&game, &deck, &player, &dealer); // dealer tries to catch up
					gameActive = false;
				}
			}
		}
		gameActive = true;
        printf("win ratio: %d/%d\n\n", game.wins(), game.games());
        thread_sleep_for(500);
	}
}
