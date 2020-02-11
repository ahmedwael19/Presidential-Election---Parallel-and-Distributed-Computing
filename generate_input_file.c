#include <stdio.h>

#include <stdlib.h>
#include <sys/resource.h>

int main(int argc, char * argv[]) {

  /* This snippet is to be able to write a very big file without a segmentation error ( core dumped) , it works by increasing the stack size */
    const rlim_t kStackSize = 48 * 1024 * 1024;   // min stack size = 48 MB
    struct rlimit rl;
    int result;
    result = getrlimit(RLIMIT_STACK, &rl);
    if (result == 0)
    {
      if (rl.rlim_cur < kStackSize)
      {
        rl.rlim_cur = kStackSize;
        result = setrlimit(RLIMIT_STACK, &rl);
        if (result != 0)
        {
          fprintf(stderr, "setrlimit returned result = %d\n", result);
        }
      }
    }

    int cands, voters;
    int swap_vote; // to randomize the votes without repeation
    printf("One second, generating the input File !\n");
    srand((unsigned) time(NULL)); // random seed
    voters = rand()%5000000; // maximum number of voters is 5 millions
    cands = rand()%6; // maximum number of cands is 6 ( to increase the chance of winning from the second round)
    int randomIndex ;  // the index that will be swapped
    if (cands < 2) 
    { // to avoid one candidate
      cands = 2;
    }
    if (voters < 20) 
    { // to avoid 20 voters or less
      voters = 20;
    }
    int elections_matrix[voters][cands];
    int counter = 1; // to populate the election matrix
    for (int i = 0; i < voters; i++) 
    {
      for (int j = 0; j < cands; j++) 
      {
        elections_matrix[i][j] = counter;
        counter++;
      }
      counter = 1;
    }

    FILE * out = fopen("elections.txt", "w+"); // open the file or create it if it is not there
    if (out == NULL) 
    {
      printf("Error --> Can't open file\n");
      exit(1);
    }
    // write the number of voters first and then the candidates in the second line
    fprintf(out, "%d", voters);
    fprintf(out, "\n");
    fprintf(out, "%d", cands);
    fprintf(out, "\n");

    for (int i = 0; i < voters; i++) 
    {
      for (int k = 0; k < cands; k++) 
      { // swap and randomize without reapeation
        swap_vote = elections_matrix[i][k]; 
        randomIndex = rand() % cands;
        elections_matrix[i][k] = elections_matrix[i][randomIndex];
        elections_matrix[i][randomIndex] = swap_vote;
      }
    }
    for (int i = 0; i < voters; i++) {
      for (int j = 0; j < cands; j++) {
        fprintf(out, "%d ", elections_matrix[i][j]); // write to the file
      } 
      fprintf(out, "\n"); 
    }
    fclose(out);
    printf("DONE ! Input file has been generated with the name elections.txt \n");
    exit(1);
  }

