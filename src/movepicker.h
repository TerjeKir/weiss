// movepicker.h

#include "types.h"


typedef enum MPStage {
    TTMOVE, GEN_NOISY, NOISY, GEN_QUIET, QUIET
} MPStage;

typedef struct MovePicker {
    Position *pos;
    MoveList *list;
    MPStage stage;
    int ttMove;
    bool onlyNoisy;
} MovePicker;


int NextMove(MovePicker *mp);