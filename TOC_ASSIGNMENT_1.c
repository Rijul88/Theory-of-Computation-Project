#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <signal.h>

#define MAX_STATES 500 

// Calculate the current state given coordinates (x, y) and grid size n
int calculateCurrState(int x, int y, int n) {
    return x + ((n + 1) * y);
}

// Calculate possible states for non-boundary coordinates
int* calculatePossibleStates(int x, int y, int n, int index) {
    int* states = (int*)malloc(2 * sizeof(int));
    if (index == 0) {
        states[0] = calculateCurrState(x - 1, y, n);
        states[1] = calculateCurrState(x + 1, y, n);
    } else {
        states[0] = calculateCurrState(x, y - 1, n);
        states[1] = calculateCurrState(x, y + 1, n);
    }
    return states;
}

// Calculate possible states for boundary coordinates
int* calculatePossibleState(int x, int y, int n, int index) {
    int* states = (int*)malloc(2 * sizeof(int));
    int N = n + 1;

    if (index == 0) {
        if (x % N == 0) {
            states[0] = calculateCurrState(x + 1, y, n);
        } else if (x % N == n) {
            states[0] = calculateCurrState(x - 1, y, n);
        }
    } else {
        if (y % N == 0) {
            states[0] = calculateCurrState(x, y + 1, n);
        } else if (y % N == n) {
            states[0] = calculateCurrState(x, y - 1, n);
        }
    }

    states[1] = -1; // Indicates a single state
    return states;
}

// Define a structure for DFA states
typedef struct {
    int states[MAX_STATES]; // DFA states are comprised of multiple states => an array of states
    int count; // # of states in the particular DFA State
} DFAState;

// Function to check if a DFA state already exists
int findDFAState(DFAState* dfaStates, int numDFAStates, DFAState newState) {
    for (int i = 0; i < numDFAStates; i++) {
        if (dfaStates[i].count == newState.count) {
            int match = 1;
            for (int j = 0; j < newState.count; j++) {
                if (dfaStates[i].states[j] != newState.states[j]) {
                    match = 0;
                    break;
                }
            }
            if (match) return i;
        }
    }
    return -1;
}

// Function to convert NFA to DFA and print transitions
void convertNFAToDFA(int* nfa[][2], int numNFAStates, int finalNFAState) {
    DFAState dfaStates[MAX_STATES];
    int numDFAStates = 0;
    int finalDFAStates[MAX_STATES];
    int numFinalDFAStates = 0;

    // Start with the initial DFA state (corresponding to the initial NFA state 0)
    dfaStates[numDFAStates].states[0] = 0;
    dfaStates[numDFAStates].count = 1;
    numDFAStates++;

    for (int i = 0; i < numDFAStates; i++) {
        for (int input = 0; input < 2; input++) {
            DFAState newState;
            newState.count = 0;

            for (int j = 0; j < dfaStates[i].count; j++) {
                int nfaState = dfaStates[i].states[j];
                int* transitions = nfa[nfaState][input];

                for (int k = 0; ((k < 2) && (transitions[k] != -1)); k++) {
                    int state = transitions[k];

                    // Add state to newState if not already present
                    int found = 0;
                    for (int l = 0; l < newState.count; l++) {
                        if (newState.states[l] == state) {
                            found = 1;
                            break;
                        }
                    }
                    if (!found) {
                        newState.states[newState.count] = state;
                        newState.count++;
                    }
                }
            }

            // Sort newState.states for easier comparison
            for (int l = 0; l < newState.count - 1; l++) {
                for (int m = l + 1; m < newState.count; m++) {
                    if (newState.states[l] > newState.states[m]) {
                        int temp = newState.states[l];
                        newState.states[l] = newState.states[m];
                        newState.states[m] = temp;
                    }
                }
            }

            // Check if the new state already exists in the DFA
            int stateIndex = findDFAState(dfaStates, numDFAStates, newState);

            if (stateIndex == -1) {
                // Add new state to DFA
                dfaStates[numDFAStates] = newState;
                stateIndex = numDFAStates;
                numDFAStates++;

                // Check if the new DFA state is a final state
                for (int l = 0; l < newState.count; l++) {
                    if (newState.states[l] == finalNFAState) {
                        finalDFAStates[numFinalDFAStates] = numDFAStates - 1;
                        numFinalDFAStates++;
                        break;
                    }
                }
            }

            // Print DFA transition
            printf("DFA State %d ----%d----> DFA State %d (NFA states: {", i, input, stateIndex);
            for (int l = 0; l < newState.count; l++) {
                printf("%d", newState.states[l]);
                if (l < newState.count - 1) printf(", ");
            }
            printf("})\n");
        }
    }

    // Print final states of the DFA
    printf("\nFinal states of the DFA:\n");
    for (int i = 0; i < numFinalDFAStates; i++) {
        printf("DFA State %d\n", finalDFAStates[i]);
    }
}


void handle_sigterm(int signum) {
    // Exit immediately on SIGTERM
    _exit(0);
}

int main() {

    // TASK 2

    int status = 0;
    int parentID = getppid();
    int n;
    char input[1000]; // Assuming input string will not be longer than 999 characters

    // Read n and input from file "input.txt"
    FILE *file = fopen("input.txt", "r");
    if (file == NULL) {
        perror("Failed to open input file");
        exit(EXIT_FAILURE);
    }

    if (fscanf(file, "%d\n%s", &n, input) != 2) {
        perror("Failed to read n and input from file");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    fclose(file);

    int N = n + 1;
    int itr = 0;
    int i = 0, j = 0;
    int state;
    int *flag;
    int rows = N * N;
    int* nfa[rows][2];

    // Initialize the nfa array with NULL
    for (int k = 0; k < rows; k++) {
        nfa[k][0] = NULL;
        nfa[k][1] = NULL;
    }

    // Construct the NFA
    for (int j = 0; j < N; j++) {
        for (int i = 0; i < N; i++) {
            state = calculateCurrState(i, j, n);
            if ((i % N == 0 && j % N == 0) || (i % N == 0 && j % N == n) || 
                (i % N == n && j % N == 0) || (i % N == n && j % N == n)) {
                nfa[state][0] = calculatePossibleState(i, j, n, 0);
                nfa[state][1] = calculatePossibleState(i, j, n, 1);
            } else if (i % N == 0 || i % N == n) {
                nfa[state][0] = calculatePossibleState(i, j, n, 0);
                nfa[state][1] = calculatePossibleStates(i, j, n, 1);
            } else if (j % N == 0 || j % N == n) {
                nfa[state][0] = calculatePossibleStates(i, j, n, 0);
                nfa[state][1] = calculatePossibleState(i, j, n, 1);
            } else {
                nfa[state][0] = calculatePossibleStates(i, j, n, 0);
                nfa[state][1] = calculatePossibleStates(i, j, n, 1);
            }
        }
    }

    printf("NFA Transitions:\n\n");

    // Print the NFA transitions
    for (int k = 0; k < rows; k++) {
        for (int l = 0; l < 2; l++) {
            if (nfa[k][l] != NULL) {
                if (nfa[k][l][1] == -1) {
                    printf("NFA State %d ----%d----> NFA State %d\n", k, l, nfa[k][l][0]);
                } else {
                    printf("NFA State %d ----%d----> NFA States %d, %d\n", k, l, nfa[k][l][0], nfa[k][l][1]);
                }
            }
        }
    }

    // The final state of the NFA is the top-right corner of the grid.
    int finalNFAState = calculateCurrState(n, n, n);

    // Convert NFA to DFA and print transitions
    printf("\nDFA Transitions:\n\n");
    convertNFAToDFA(nfa, rows, finalNFAState);

    // Free allocated memory
    for (int k = 0; k < rows; k++) { 
        for (int l = 0; l < 2; l++) {
            if (nfa[k][l] != NULL) {
                free(nfa[k][l]);
            }
        }
    }

    // TASK 1
    
    state = 0;

    // Create shared memory segment for the flag
    flag = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (flag == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Initializing the flag to 0
    *flag = 0;

    char output[1000] = "";
    pid_t parent_pid = getpid();
    int numStates = strlen(input) + 1;
    int stateItr = 1;

    printf("\nTraversal for the input string %s is as follows:\n\n",input);

    // Set the process group ID to the parent's Process ID:
    setpgid(0, 0);

    // Set up signal handler for SIGTERM
    signal(SIGTERM, handle_sigterm);

    while (stateItr <= numStates) {
        char buffer[20];
        state = i + (n + 1) * j;
        snprintf(buffer, sizeof(buffer), "%d ", state);
        strcat(output, buffer);

        if ((state == (N * N) - 1) && (itr == strlen(input))) {
            *flag = 1;
            // Send SIGTERM to all processes in the process group
            printf("Process ID:: %d, States: %s, String accepted.\n", getpid(), output);

            killpg(0, SIGTERM);
            break;
        } else if (*flag == 0) {
            if ((i % N == 0) && (input[itr] == '0')) {
                i = i + 1;
                itr += 1;
                if (*flag == 1) handle_sigterm(SIGTERM);
                if (stateItr < numStates) printf("Process ID:: %d, States: %s\n", getpid(), output);
                else printf("Process ID:: %d, States: %s,String not accepted.\n", getpid(), output);
                stateItr++;
                continue;
            } else if ((i % N == n) && (input[itr] == '0')) {
                i = i - 1;
                itr += 1;
                if (*flag == 1) handle_sigterm(SIGTERM);
                if (stateItr < numStates) printf("Process ID:: %d, States: %s\n", getpid(), output);
                else printf("Process ID:: %d, States: %s,String not accepted.\n", getpid(), output);
                stateItr++;
                continue;
            } else if ((j % N == 0) && (input[itr] == '1')) {
                j = j + 1;
                itr += 1;
                if (*flag == 1) handle_sigterm(SIGTERM);
                if (stateItr < numStates) printf("Process ID:: %d, States: %s\n", getpid(), output);
                else printf("Process ID:: %d, States: %s,String not accepted.\n", getpid(), output);
                stateItr++;
                continue;
            } else if ((j % N == n) && (input[itr] == '1')) {
                j = j - 1;
                itr += 1;
                if (*flag == 1) handle_sigterm(SIGTERM);
                if (stateItr < numStates) printf("Process ID:: %d, States: %s\n", getpid(), output);
                else printf("Process ID:: %d, States: %s,String not accepted.\n", getpid(), output);
                stateItr++;
                continue;
            } else {
                int id = fork();
                if (id != 0) {
                    // Parent process
                    int new_itr = itr;
                    if (input[new_itr] == '0') {
                        i = i + 1;
                        itr += 1;
                        if (*flag == 1) handle_sigterm(SIGTERM);
                        if (stateItr < numStates) printf("Process ID:: %d, States: %s\n", getpid(), output);
                        else printf("Process ID:: %d, States: %s,String not accepted.\n", getpid(), output);
                        stateItr++;
                        continue;
                    } else {
                        j = j + 1;
                        itr += 1;
                        if (*flag == 1) handle_sigterm(SIGTERM);
                        if (stateItr < numStates) printf("Process ID:: %d, States: %s\n", getpid(), output);
                        else printf("Process ID:: %d, States: %s,String not accepted.\n", getpid(), output);
                        stateItr++;
                        continue;
                    }
                } else {
                    // Child process
                    // Set the process group ID to the parent's Process ID:
                    setpgid(0, parent_pid);
                    int new_itr = itr;
                    if (input[new_itr] == '0') {
                        i = i - 1;
                        itr += 1;
                        if (*flag == 1) handle_sigterm(SIGTERM);
                        if (stateItr < numStates) printf("Process ID:: %d, States: %s\n", getpid(), output);
                        else printf("Process ID:: %d, States: %s,String not accepted.\n", getpid(), output);
                        stateItr++;
                        continue;
                    } else {
                        j = j - 1;
                        itr += 1;
                        if (*flag == 1) handle_sigterm(SIGTERM);
                        if (stateItr < numStates) printf("Process ID:: %d, States: %s\n", getpid(), output);
                        else printf("Process ID:: %d, States: %s,String not accepted.\n", getpid(), output);
                        stateItr++;
                        continue;
                    }
                }
            }
        }
    }
    while (wait(NULL) > 0);
    if (getppid() == parentID) {
        
        if (*flag == 0)
            printf("NOT A VALID STRING!");
    }

    // Clean up shared memory
    munmap(flag, sizeof(int));
    return 0;
}
