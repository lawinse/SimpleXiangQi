#ifndef SEARCH_H
#define SEARCH_H
#include <windows.h>
#include "GUI.H"
#include "BASE.H"
#include "HELPER.H"

struct HashStruct {
	BYTE ucDepth, ucFlag;
	int svl;
	WORD wmv, wReserved;
	DWORD lock0, lock1;
};

struct OpenBookStruct {
	DWORD lock;
	WORD wmv, wvl;
};

struct Search {
	int mvResult, nBookSize;
	int nHistoryTable[65536];
	int mvKillers[LIMIT_DEPTH][2];
	HashStruct hashTable[HASH_SIZE];
	OpenBookStruct opBookTable[BOOK_SIZE];
};

extern Search SEARCH;
extern Position POS;

void LoadBook();
static int CompareBook(const void *bk1, const void *bk2);
static int SearchBook();
static int ProbeHash(int alpha, int beta, int nDepth, int &mv);
static void RecordHash(int nFlag, int value, int nDepth, int mv);


/**
* SORT 
*/

int Mvvlva(int mv);

bool CompareMvvLva(const int &mv1, const int &mv2);
bool CompareHistory(const int &h1, const int &h2);


class MoveSort {
	static const int PHASE_HASH = 0;
	static const int PHASE_KILLER0 = 1;
	static const int PHASE_KILLER1 = 2;
	static const int PHASE_GEN_MOVES = 3;
	static const int PHASE_REST = 4;

	int mvHash, mvKiller[2];
	int nPhase, nIndex, nGenMoves;
	int mvs[MAX_GEN_MOVES];
public:
	void init(int _mvhsh);
	int getNext();
};

void SetBestMove(int mv, int nDepth);
static int SearchQuiesc(int alpha, int beta);

const bool NO_NULL = true;
static int SearchFull(int alpha, int beta, int nDepth, bool isNoNull=false);
static int SearchRoot(int nDepth);
void SearchMain();

#endif