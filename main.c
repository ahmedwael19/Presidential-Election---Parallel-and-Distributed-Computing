#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include "mpi.h"

int main(int argc, char * argv[]) {
  /* Declare the global Variables */
    int my_rank; /*rank of processes*/
    int p; /*number of processes*/
    int tag = 0; /*tag for messages*/
    int offset; /* To specifiy from where we should be reading the file */
    int start_offset=0; /* For reading the file starting from the voters and skip the number of cands and voters */
    int voters_per_p; /*Stores the number of voters that each of the processes will work on*/
    int remaining_voters; /*Stores the extra amount of voters that won't be divided and will be added to the work of the master process*/
    int lowerbound; /*Stores the lower bound of voters for nonmaster processes*/
    int upperbound; /*Stored the upper bound of voters for non master processes*/
    MPI_Status status; /*return status for*/
  int voters, cands; 
    int current_vote; /* The current vote taken from the file ( copy for each process ) */
    int range; /* Range of candidates for each process */
    int temp_count; /* Which candidate will have the current vote */

  /* Round 1 Variables */
    int *partial_list_round1; /* Dynamically Allocated 2D array for the partial list done by each process ( first round) */
    int * partial_count_round1; /* Dynamically allocated 1D array containing the partial count calculated from each process (for first round) */
    float * percentages_round1; /* Dynamically Allocated 1D array storing the percentage of each candidate */
    int * totalcount_round1; /* Dynamically allocated 1D array containing the total number of votes every candidate had */
    int flag_round2=0; // 0 indicates that need to start round 2, 1 indicates the end of election from the first round
    int cand1_round2, cand2_round2; /* The winners of the first round */

    /*Start up MPI*/
    MPI_Init( & argc, & argv);
    /*find out process rank*/
    MPI_Comm_rank(MPI_COMM_WORLD, & my_rank);
    /*find out number of processes*/
    MPI_Comm_size(MPI_COMM_WORLD, & p);

    /* Now we need to divide the number of voters among the available processes to work on */

    /*Let the master process do this part*/

    if (p == 1 && my_rank == 0) 
    {
      FILE * file_copy = fopen("elections.txt", "r");

      /*Code for one process*/

      /*this part should handle running the code with a single process*/
      fscanf(file_copy, "%d", & voters);
      fscanf(file_copy, "%d", & cands);
      printf("We have %d Candidates and %d Voters \n\n", cands,voters);
      lowerbound = 0;
      upperbound = voters;
      totalcount_round1 = (int * ) malloc(cands * sizeof(int));
      partial_list_round1 = (int * ) malloc((voters * cands) * sizeof(int * ));
      percentages_round1 = (float * ) malloc(cands * sizeof(int));
      /*Next compute the full solution of the master process*/
      for (int i = 0; i < cands; i++) 
      {
        totalcount_round1[i] = 0; // populate with zeros
      }

      for (int i = 0; i < voters; i++) 
      {
        for (int j = 0; j < cands; j++) 
        {
          fscanf(file_copy, "%d", & current_vote);
          partial_list_round1[i * cands + j] = current_vote; /* Pouplate the partial list matrix with values from the file */
        }
        temp_count = partial_list_round1[i * cands]; // add the vote to the candidate
        totalcount_round1[temp_count - 1] = totalcount_round1[temp_count - 1] + 1;
      }

      /*Now print totalcount_round1 for first trial*/
      int k;
      for (k = 0; k < cands; k++) 
      {
        percentages_round1[k] = (float)totalcount_round1[k] * 100 / voters;
        printf("Candidate %i received %i votes out of %i votes, which is %0.2f percent of the votes.\n", k + 1, totalcount_round1[k], voters, percentages_round1[k]);
      }
    } 
    /* If more than one process is running */

    else 
    {
      if (my_rank == 0) // master code
      {
        FILE * file_copy = fopen("elections.txt", "r"); // for each process, open the file.

        fscanf(file_copy, "%d", & voters);
        fscanf(file_copy, "%d", & cands);
        printf("We have %d Candidates and %d Voters \n\n", cands,voters);
        voters_per_p = voters / p;
        remaining_voters = voters % p;
        // Get the number of digits for the voters --> So we can calculate ths start offset
        int number_of_digits =voters;
        while (number_of_digits != 0) { 
          number_of_digits = number_of_digits / 10; 
          ++start_offset; 
        } 
        start_offset+=2;
        offset = cands*2+1;
        /* Dynamically allocate the arrays */
        totalcount_round1 = (int * ) malloc(cands * sizeof(int));
        percentages_round1 = (float * ) malloc(cands * sizeof(int));
        partial_count_round1 = (int * ) malloc(cands * sizeof(int));
        printf("Each node will work on %i voters \n", voters_per_p);
        printf("The remaining work for master is %i \n", remaining_voters);
        fclose(file_copy);
      }
    /* Broadcast the candidates, voters, voters per process,start offset, and offset to all processes */
      MPI_Bcast( & cands, 1, MPI_INT, 0, MPI_COMM_WORLD);
      MPI_Bcast( & voters, 1, MPI_INT, 0, MPI_COMM_WORLD);
      MPI_Bcast( & voters_per_p, 1, MPI_INT, 0, MPI_COMM_WORLD);
      MPI_Bcast( & start_offset, 1, MPI_INT, 0, MPI_COMM_WORLD);
      MPI_Bcast( & offset, 1, MPI_INT, 0, MPI_COMM_WORLD);
      MPI_Barrier(MPI_COMM_WORLD);
      if (my_rank != 0) 
    { // workers code
      lowerbound = ((my_rank - 1) * voters_per_p) ;
      upperbound = my_rank * voters_per_p;
      range = upperbound - lowerbound;
      printf("Process %d will start from Voter number %d to Voter number %d\n",my_rank,lowerbound,upperbound);
      partial_list_round1 = (int * ) malloc((range * cands) * sizeof(int * ));
      partial_count_round1 = (int * ) malloc(cands * sizeof(int));
      /*First read the respective part of the file*/
      FILE * file_copy = fopen("elections.txt", "r"); // for each process, open the file.
      fseek(file_copy, start_offset + (offset * (lowerbound)), SEEK_SET);
      int temp_count;
      for (int i = 0; i < cands; i++) 
      {
        partial_count_round1[i] = 0;
      }
      for (int i = 0; i < range; i++) 
      {
        for (int j = 0; j < cands; j++) 
        {
          fscanf(file_copy, "%d", & current_vote);
          partial_list_round1[i * cands + j] = current_vote;
        }
        temp_count = partial_list_round1[i * cands];
        partial_count_round1[temp_count - 1]++;
      }
      fclose(file_copy);
      /*next each process should send out the generated partial count to the master process*/
      MPI_Send(  partial_count_round1, cands, MPI_INT, 0, tag, MPI_COMM_WORLD);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    if (my_rank == 0) 
    {
      lowerbound = voters - (remaining_voters + voters_per_p);
      upperbound = voters;
      range = upperbound - lowerbound;
      int temp_count;
      printf("Process %d will start from Voter number %d to Voter number %d\n",my_rank,lowerbound,upperbound);
      partial_list_round1 = (int * ) malloc((range * cands) * sizeof(int * ));
      partial_count_round1 = (int * ) malloc(cands * sizeof(int));
        FILE * file_copy = fopen("elections.txt", "r"); // for each process, open the file.
        fseek(file_copy, start_offset + (offset * (lowerbound)), SEEK_SET);

        for (int i = 0; i < cands; i++) 
        {
          totalcount_round1[i] = 0;
        }
        for (int i = 0; i < range; i++) 
        {
          for (int j = 0; j < cands; j++) 
          {
            fscanf(file_copy, "%d", & current_vote);
            partial_list_round1[i * cands + j] = current_vote;
          }
          temp_count = partial_list_round1[i * cands];
          totalcount_round1[temp_count - 1] = totalcount_round1[temp_count - 1] + 1;
        }
        fclose(file_copy);
        /*Now receive the partial count generated by each of the processes & add them all together*/
        for (int source = 1; source < p; source++) 
        {
          MPI_Recv( partial_count_round1, cands, MPI_INT, source, tag, MPI_COMM_WORLD, & status);
          for (int k = 0; k < cands; k++) 
          {
            totalcount_round1[k] = totalcount_round1[k] + partial_count_round1[k];
          }
        }
        /*Now print totalcount_round1 for first trial*/
        for (int k = 0; k < cands; k++) 
        {
          percentages_round1[k] = (float)totalcount_round1[k] * 100 / voters;
          printf("Candidate %i received %i votes out of %i votes, which is %0.2f percent of the votes.\n", k + 1, totalcount_round1[k], voters, percentages_round1[k]);
        }
      }
    }
    MPI_Barrier(MPI_COMM_WORLD);

    if (my_rank == 0) 
    {
        /*Now the master process will work on generating the results of the elections based on the previous results*/
        /*First if one of the vadidates receives more than half of the votes, he wins the elections without the need to go through the second round*/
      for (int i = 0; i < cands; i++) 
      {
        if (percentages_round1[i] > 50) 
        {
          printf("Candidate %i wins the elections with %0.2f percent of the votes.\n", i + 1, percentages_round1[i]);
          printf("No need to go through second round of elections. \n");
          flag_round2 = 1;
          /*Shutdown MPI*/
          MPI_Finalize();
          return 0;
        }
      }

      if (flag_round2 == 0) 
      {
          /*None of the candidates have received more than 50% of the votes*/
          /*Need to choose two candidates with the highest number of votes for round two*/
        int temp_count = 0;
        for (int i = 0; i < cands; i++) 
        {
            /*Search for the candidate that received the largest no of votes*/
          if (totalcount_round1[i] > temp_count) 
          {
            temp_count = totalcount_round1[i];
            cand1_round2 = i + 1;
          }
        }
        temp_count = 0;
        totalcount_round1[cand1_round2-1]=0; // set the largest to zero and repeat the algorithm to get the second largest
        for (int i = 0; i < cands; i++) 
        {
            /*Search for the candidate that received the second largest no of votes*/
          if (totalcount_round1[i] >= temp_count ) 
          {
            temp_count = totalcount_round1[i];
            cand2_round2 = i + 1;
          }
        }

        printf("The candidates no %i & %i will move on to the second round of the elections.\n", cand1_round2, cand2_round2);
        for ( int i = 1; i<p;i++)
        {
          // Send the winners and the flag
          MPI_Send( &cand1_round2, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
          MPI_Send( &cand2_round2, 1, MPI_INT, i, tag,MPI_COMM_WORLD);
          MPI_Send( &flag_round2, 1, MPI_INT, i,tag, MPI_COMM_WORLD);
        }
      }

    }
    MPI_Barrier(MPI_COMM_WORLD);

  /* Second Round of Election */ 
  /* Second round Variables */
    int * partial_list_round2_master_only; /* Dynamically allocated 2D array for the partial list in the second round in case of one process */
    int * partial_list_round2;/* Dynamically allocated 2D array for the partial list in the second round */
    int cand1_partial_votes,cand2_partial_votes; /* How many votes the first and the second candidate had in the second round */
    int partial_count_round2[2]={0,0};
    float percentages_round2[2];
    int totalcount_round2[2]={0,0};

    if (my_rank==0 && flag_round2==0 && p==1)
    {
      FILE * file_copy = fopen("elections.txt", "r"); // for each process, open the file.

      /*this part should handle running the code with a single process*/
      fscanf(file_copy, "%d", & voters);
      fscanf(file_copy, "%d", & cands);
      printf("We have %d Candidates and %d Voters \n", cands,voters);
      lowerbound = 0;
      upperbound = voters;
      totalcount_round1 = (int * ) malloc(cands * sizeof(int));
      partial_list_round2_master_only = (int * ) malloc((voters * 2) * sizeof(int * ));
      int temp_count;

      int voter_index = 0;

      for (int i = 0; i < voters; i++) 
      {
        for (int j = 0; j < cands; j++) 
        {
        fscanf(file_copy, "%d", & current_vote); // get the current vote
        if((current_vote == cand1_round2) || (current_vote == cand2_round2)) // if it is one of the winners of the first round, add it to the partial list in the place of this candidate and increase the voter index
        {
          partial_list_round2_master_only[i*2 + voter_index] = current_vote;
          voter_index++;
        }
      }
      temp_count = partial_list_round2_master_only[i*2]; // get the first index of this voter ( the first choice)
      if(temp_count == cand1_round2) // increment the total count of either 
      {
        totalcount_round2[0]++;
      }
      else if (temp_count ==cand2_round2)
      {
        totalcount_round2[1]++;
      }
      voter_index=0;
    }

    fclose(file_copy);
    /*Now print totalcount_round1 for second trial*/
    int tiger[2]={cand1_round2,cand2_round2};
    printf(" \nAccording to the conditions of round 2 of the election : \n");
    for (int k = 0; k < 2; k++) 
    {
      percentages_round2[k] = (float)totalcount_round2[k] * 100 / voters;

      printf("Candidate %i received %i votes out of %i votes, which is %.2f percent of the votes.\n", tiger[k], totalcount_round2[k], voters, percentages_round2[k]);
    }
    for (int i = 0; i < 2; i++) 
    {
      if (percentages_round2[i] > 50) 
      {
        printf("Candidate %i wins the elections with %.2f percent of the votes.\n", tiger[i], percentages_round2[i]);
        printf("CONGRATULATIONS ! candidate %d ! you are now the president  !!!!!!!!!! \n",tiger[i]);
            /*Shutdown MPI*/
        MPI_Finalize();
        return 0;
      }
    }
  }
      MPI_Barrier(MPI_COMM_WORLD);

  if (my_rank != 0 && flag_round2==0) 
  {
    MPI_Recv( &cand1_round2, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, & status);
    MPI_Recv( &cand2_round2, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, & status);
    MPI_Recv( &flag_round2, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, & status);
    FILE * file_copy = fopen("elections.txt", "r"); // for each process, open the file.
    fseek(file_copy, start_offset + (offset * (lowerbound)), SEEK_SET);
    int temp_count;
    partial_list_round2 = (int * ) malloc((voters * 2) * sizeof(int * ));

    int voter_index = 0;
    for (int i = 0; i < range; i++) 
    {
      for (int j = 0; j < cands; j++) 
      {
        fscanf(file_copy, "%d", & current_vote);
        if( (current_vote == cand1_round2) || (current_vote == cand2_round2) )
        {
          partial_list_round2[i*2 + voter_index] = current_vote;
          voter_index++;
        }
            //partial_list_round1[i * cands + j] = current_vote;
      }
      voter_index=0;
      temp_count = partial_list_round2[i*2];
      if(temp_count == cand1_round2)
      {
        partial_count_round2[0]++;
      }
      else if (temp_count ==cand2_round2)
      {
        partial_count_round2[1]++;
      }
    }
    fclose(file_copy);
    cand1_partial_votes = partial_count_round2[0];
    cand2_partial_votes = partial_count_round2[1];
    MPI_Send(  &cand1_partial_votes, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);
    MPI_Send(  &cand2_partial_votes, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);
  }
      MPI_Barrier(MPI_COMM_WORLD);

  if (my_rank==0 && flag_round2==0)
  {
    FILE * file_copy = fopen("elections.txt", "r"); // for each process, open the file.
    fseek(file_copy, start_offset + (offset * (lowerbound)), SEEK_SET);
    int temp_count;

    partial_list_round2 = (int * ) malloc((voters * 2) * sizeof(int * ));
    int voter_index = 0;
    for (int i = 0; i < range; i++) {
      for (int j = 0; j < cands; j++) {
        fscanf(file_copy, "%d", & current_vote);
        if( (current_vote == cand1_round2) || (current_vote == cand2_round2) )
        {
          partial_list_round2[i*2 + voter_index] = current_vote;
          voter_index++;
        }
      }
      voter_index=0;
      temp_count = partial_list_round2[i*2];
      if(temp_count == cand1_round2)
      {
        totalcount_round2[0]++;
      }
      else if (temp_count ==cand2_round2)
      {
        totalcount_round2[1]++;
      }
    }
    fclose(file_copy);
    for (int source = 1; source < p; source++) 
    {
      MPI_Recv(&cand1_partial_votes, 1, MPI_INT, source, tag, MPI_COMM_WORLD, &status);
      MPI_Recv(&cand2_partial_votes, 1, MPI_INT, source, tag, MPI_COMM_WORLD, &status);
      totalcount_round2[0] = totalcount_round2[0] + cand1_partial_votes;
      totalcount_round2[1] = totalcount_round2[1] + cand2_partial_votes;
    }        
    
    /*Now print totalcount_round1 for second trial*/
    int tiger[2]={cand1_round2,cand2_round2};
    printf(" \nAccording to the conditions of round 2 of the election : \n");
    for (int k = 0; k < 2; k++) 
    {
      percentages_round2[k] = (float)totalcount_round2[k] * 100 / voters;
      printf("Candidate %i received %i votes out of %i votes, which is %.2f percent of the votes.\n", tiger[k], totalcount_round2[k], voters, percentages_round2[k]);
    }
    for (int i = 0; i < 2; i++) {
      if (percentages_round2[i] > 50) {
        printf("Candidate %i wins the elections with %.2f percent of the votes.\n", tiger[i], percentages_round2[i]);
        printf("CONGRATULATIONS ! candidate %d ! you are now the president  !!!!!!!!!! \n",tiger[i]);
            /*Shutdown MPI*/
        MPI_Finalize();
        return 0;
      }
    }
  }

        /*Shutdown MPI*/
  MPI_Finalize();
  return 0;
}
