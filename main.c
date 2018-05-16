// -------------------------
// Generic Grafcet Framework
// -------------------------

// MIT License:
//
// Copyright 2018 Gonçalo Santos
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
// to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.


#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <stropts.h>

// Linux (POSIX) implementation of _kbhit().
// Morgan McGuire, morgan@cs.brown.edu
int _kbhit() {
    static const int STDIN = 0;
    static bool initialized = false;

    if (! initialized) {
        // Use termios to turn off line buffering
        struct termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = true;
    }

    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}


#define clear() printf("\033[H\033[J")

#define ArrayCount(arr) ((sizeof(arr))/sizeof(*arr))

#define STATE_OUTPUT_FUNCTION(Name) void Name()
typedef STATE_OUTPUT_FUNCTION(state_output_function);

#define TRANSITION_CONDITION_FUNCTION(Name) bool Name()
typedef TRANSITION_CONDITION_FUNCTION(transition_condition_function);

#define ARR(...) {__VA_ARGS__}

#include "preprocessor_output.h"

// NOTE(nox): The ... in the macros are the place for the Output and Condition, respectively
#define newState(Grafcet, Name_, ...)      \
    do {                                                    \
        state_id Id = State_X##Name_;                         \
        Grafcets[Grafcet].States[Grafcets[Grafcet].StateCount++] = Id; \
        strncpy(States[Id].Name, #Name_, NAME_LENGTH);             \
        States[Id].Output = stateAction_X##Name_; \
    } while(0)

#define newTransition(Grafcet, Name_, PrevStates, NextStates_, ...)   \
    do {                                                              \
        transition_id Id = Transition_##Name_;                         \
        state_id P[] = PrevStates;                                     \
        state_id N[] = NextStates_;                                     \
        Grafcets[Grafcet].Transitions[Grafcets[Grafcet].TransitionCount++] = Id; \
        strncpy(Transitions[Id].Name, #Name_, NAME_LENGTH);             \
        Transitions[Id].Condition = transitionCondition_##Name_;    \
        Transitions[Id].PreviousStatesCount = ArrayCount(P); \
        for(int I = 0; I < ArrayCount(P); ++I) { Transitions[Id].PreviousStates[I] = P[I]; } \
        Transitions[Id].NextStatesCount = ArrayCount(N); \
        for(int I = 0; I < ArrayCount(N); ++I) { Transitions[Id].NextStates[I] = N[I]; } \
    } while(0)


#define GRAFCET_COUNT 2
#define GRAFCET_MAX_STATES 1024
#define GRAFCET_MAX_TRANSITIONS 1024
#define NAME_LENGTH 1<<4

typedef struct {
    bool Active;
    char Name[NAME_LENGTH];
    uint64_t Timer;
    state_output_function *Output;
} state;

typedef struct {
    bool Active;
    char Name[NAME_LENGTH];
    int PreviousStatesCount;
    state_id PreviousStates[GRAFCET_MAX_STATES];
    int NextStatesCount;
    state_id NextStates[GRAFCET_MAX_STATES];
    transition_condition_function *Condition;
} transition;

typedef struct {
    int StateCount;
    state_id States[GRAFCET_MAX_STATES];

    int TransitionCount;
    transition_id Transitions[GRAFCET_MAX_TRANSITIONS];

    bool Frozen;
} grafcet;

static grafcet Grafcets[GRAFCET_COUNT];
static state States[StateCount];
static transition Transitions[TransitionCount];

// NOTE(nox): Inputs and Outputs
#define ioEnumWriter(Name, ...) IO_##Name

typedef struct {
    bool Active;
    char Name[25];
    char Key;
} input;

#define inputStructWriter(Name, Key) { false, #Name, Key }
#define inputMacro(W) W(QUIT, 'q'), W(M_MAX, 'a'), W(M_MIN, 's'), W(PRATO1, 'd'), W(PRATO2, 'f'), W(PARAGEM, 'p'), W(CICLO, 'c')

static input Inputs[] = { inputMacro(inputStructWriter) };
typedef enum { inputMacro(ioEnumWriter) } inputLabel;
#define input(Label) Inputs[IO_##Label].Active

typedef struct {
    bool Active;
    char Name[25];
} output;

#define outputStructWriter(Name) { false, #Name }
#define outputMacro(W) W(ESQUERDA), W(BOMBA_V5), W(MOTOR_PA), W(V1), W(V2), W(V3), W(V4), W(V5), W(V7)

static output Outputs[] = { outputMacro(outputStructWriter) };
typedef enum { outputMacro(ioEnumWriter) } outputLabel;
#define output(Label) Outputs[IO_##Label].Active = true

#define FREEZE(Id) Grafcets[Id].Frozen = true
#define ACTIVE(Name) States[State_X##Name].Active
#define TIMER(Name) States[State_X##Name].Timer

#define OUTPUTS_AND_CONDITIONS
#include "preprocessor_output.h"
#undef OUTPUTS_AND_CONDITIONS

static bool checkTransitionState(transition *Transition) {
    bool AllPrevious = true;
    for(int PrevIndex = 0; PrevIndex < Transition->PreviousStatesCount; ++PrevIndex) {
        if(!States[Transition->PreviousStates[PrevIndex]].Active) {
            AllPrevious = false;
            break;
        }
    }
    if(AllPrevious) {
        return Transition->Condition();
    }

    return false;
}

int main(int Argc, char *Argv[]) {
    // NOTE(nox): Grafcets definitions
    newState(0, 1, { });
    newState(0, 2, { FREEZE(1); });
    newTransition(0, t0, ARR(State_X1), ARR(State_X2), (input(M_MAX)));
    newTransition(0, t1, ARR(State_X2), ARR(State_X1), (input(M_MIN)));

    newState(1, 3, { output(ESQUERDA); });
    newState(1, 4, { output(BOMBA_V5); });
    newTransition(1, t2, ARR(State_X3), ARR(State_X4), (TIMER(3) >= 10));
    newTransition(1, t3, ARR(State_X4), ARR(State_X3), (TIMER(4) >= 10));

    // NOTE(nox): Set default
    States[State_X1].Active = true;
    States[State_X3].Active = true;

    // NOTE(nox): Generic grafcet logic
    for(;;) {
        if(input(QUIT)) {
            break;
        }
        // NOTE(nox): Reset outputs
        for(int Index = 0; Index < ArrayCount(Inputs); ++Index) {
            Outputs[Index].Active = false;
        }

        // NOTE(nox): Read inputs
        while(_kbhit()) {
            char C = getchar();
            for(int Index = 0; Index < ArrayCount(Inputs); ++Index) {
                if(Inputs[Index].Key == C) {
                    Inputs[Index].Active = !Inputs[Index].Active;
                    break;
                }
            }
        }

        // NOTE(nox): Update timers
        for(int Index = 0; Index < StateCount; ++Index) {
            state *State = States + Index;
            if(State->Active) {
                ++State->Timer;
            }
        }

        clear();
        for(int GrafcetId = 0; GrafcetId < GRAFCET_COUNT; ++GrafcetId) {
            grafcet *Grafcet = Grafcets + GrafcetId;
            // NOTE(nox): Calculate transitions
            for(int Index = 0; Index < Grafcet->TransitionCount; ++Index) {
                transition *Transition = Transitions + Grafcet->Transitions[Index];
                Transition->Active = Grafcet->Frozen ? false : checkTransitionState(Transition);
            }

            // NOTE(nox): Deactivate above
            for(int Index = 0; Index < Grafcet->TransitionCount; ++Index) {
                transition *Transition = Transitions + Grafcet->Transitions[Index];
                if(Transition->Active) {
                    for(int PrevIndex = 0; PrevIndex < Transition->PreviousStatesCount; ++PrevIndex) {
                        States[Transition->PreviousStates[PrevIndex]].Active = false;
                    }
                }
            }

            // NOTE(nox): Activate below
            for(int Index = 0; Index < Grafcet->TransitionCount; ++Index) {
                transition *Transition = Transitions + Grafcet->Transitions[Index];
                if(Transition->Active) {
                    for(int NextIndex = 0; NextIndex < Transition->NextStatesCount; ++NextIndex) {
                        States[Transition->NextStates[NextIndex]].Active = true;
                        States[Transition->NextStates[NextIndex]].Timer = 0;
                    }
                }
            }

            // NOTE(nox): Outputs
            for(int Index = 0; Index < Grafcet->StateCount; ++Index) {
                state *State = States + Grafcet->States[Index];
                if(State->Active) {
                    State->Output();
                }
            }

            // NOTE(nox): Print debug information
            printf("Grafcet %d     States:\n", GrafcetId);
            for(int Index = 0; Index < Grafcet->StateCount; ++Index) {
                printf("%5s: %s\n", States[Grafcet->States[Index]].Name,
                       States[Grafcet->States[Index]].Active ? "Active" : "Inactive");
            }
            puts("");

            // NOTE(nox): Disable freeze
            Grafcet->Frozen = false;
        }

        printf("Inputs:\n");
        for(int Index = 1; Index < ArrayCount(Inputs); ++Index) {
            printf("%10s (%c): %s\n", Inputs[Index].Name, Inputs[Index].Key, Inputs[Index].Active ? "Active" : "Inactive");
        }
        puts("");
        printf("Outputs:\n");
        for(int Index = 0; Index < ArrayCount(Outputs); ++Index) {
            printf("%10s: %s\n", Outputs[Index].Name, Outputs[Index].Active ? "Active" : "Inactive");
        }

        puts("\n");
        usleep(100000);
    }

    return 0;
}
