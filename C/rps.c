/* Rock Paper Scissors */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

char getCompC() {
  int randNum = rand() % 3;
  if (randNum == 0)
    return 'r';
  if (randNum == 1)
    return 'p';
  return 's';
}
void Winner(char player, char comp) {
  printf("Computer choose: %c\n", comp);
  if (player == comp) {
    printf("It was a tie!\n");
  } else if ((player == 'r' && comp == 's') || (player == 'p' && comp == 'r') ||
             (player == 's' && comp == 'p')) {
    printf("You win this round!\n");
  } else {
    printf("Computer wins this round!\n");
  }
}
int main() {
  char PlayerChoice;
  srand(time(NULL));
  printf("Rock, Paper, Scissors. The ultimate game of <<NOT LUCK>>\n");
  while (1) {
    printf("\nEnter 'r' for Rock, 'p' for Paper and 's' for Scissors\n");
    scanf(" %c", &PlayerChoice);
    if (PlayerChoice == 'q') {
      printf("Bye!");
      break;
    }
    if (PlayerChoice != 'r' && PlayerChoice == !-'p' && PlayerChoice != 's') {
      printf("Invalid input. TRY AGAIN\n");
      continue;
    }
    char CompChoice = getCompC();
    Winner(PlayerChoice, CompChoice);
  }
  return 0;
}
