#include "SEARCH.H"
#include "BOARD.H"
#include <algorithm>
#include <ctime>

void LoadBook() {
	HRSRC hrsrc;
	hrsrc = FindResource(GUI.hInst, "BOOK_DATA", RT_RCDATA);
	SEARCH.nBookSize = SizeofResource(GUI.hInst, hrsrc) / sizeof(OpenBookStruct);
	if (SEARCH.nBookSize > BOOK_SIZE) {
		SEARCH.nBookSize = BOOK_SIZE;
	}
	memcpy(SEARCH.opBookTable, LockResource(LoadResource(GUI.hInst, hrsrc)),
		SEARCH.nBookSize * sizeof(OpenBookStruct));
}

int CompareBook(const void *bk1, const void *bk2) {
	DWORD lk1,lk2;
	lk1 = ((OpenBookStruct*) bk1)->lock;
	lk2 = ((OpenBookStruct*) bk2)->lock;
	return lk1 > lk2 ? 1 : lk1 < lk2 ? -1 : 0;
}

int SearchBook() {
	int i, value, nBookMoves, mv;
	int mvs[MAX_GEN_MOVES], values[MAX_GEN_MOVES];
	bool isMirror;
	OpenBookStruct bkToSearch, *pbk;
	Position posMirror;


	if (SEARCH.nBookSize == 0) return 0;

	isMirror = false;
	bkToSearch.lock = POS.getZobr().getLock(1);
	pbk = (OpenBookStruct *) bsearch(&bkToSearch, SEARCH.opBookTable,
		SEARCH.nBookSize,sizeof(OpenBookStruct),CompareBook);
	if (!pbk) {
		isMirror = true;
		POS.mirror(posMirror);
		bkToSearch.lock = posMirror.getZobr().getLock(1);
		pbk = (OpenBookStruct *) bsearch(&bkToSearch, SEARCH.opBookTable,
		SEARCH.nBookSize,sizeof(OpenBookStruct),CompareBook);
	}
	if (!pbk) return 0;


	while(pbk >= SEARCH.opBookTable && pbk->lock == bkToSearch.lock) {
		pbk--;
	}

	value = nBookMoves = 0;
	while(pbk < SEARCH.opBookTable + SEARCH.nBookSize && pbk->lock == bkToSearch.lock) {
		mv = (isMirror ? getMirrorMove(pbk->wmv) : pbk->wmv);
		if (POS.isLegalMove(mv)) {
			mvs[nBookMoves] = mv;
			values[nBookMoves] = pbk->wvl;
			value += values[nBookMoves++];
			if (nBookMoves == MAX_GEN_MOVES) break;
		}
		++pbk;
	}

	if (value == 0) return 0;

	value = rand() % value;
	for (i=0; i<nBookMoves; ++i) {
		value -= values[i];
		if (value < 0) break;
	}
	return mvs[i];
}

int ProbeHash(int alpha, int beta, int nDepth, int &mv) {
	bool isMate;
	HashStruct hsh;

	hsh = SEARCH.hashTable[POS.getZobr().getKey() & (HASH_SIZE-1)];
	if (hsh.lock0 != POS.getZobr().getLock(0) || hsh.lock1 != POS.getZobr().getLock(1)) {
		mv = 0;
		return -MATE_VALUE;
	}
	mv = hsh.wmv;
	isMate = false;
	if (hsh.svl > WIN_VALUE) {
		if (hsh.svl < BAN_VALUE) return -MATE_VALUE;
		isMate = true;
		hsh.svl -= POS.get_nDistance();
	} else if (hsh.svl < -WIN_VALUE) {
		if (hsh.svl > -BAN_VALUE) return -MATE_VALUE;
		isMate = true;
		hsh.svl += POS.get_nDistance();
	}

	if (hsh.ucDepth >= nDepth || isMate) {
		if (hsh.ucFlag == HASH_BETA) {
			return (hsh.svl >= beta ? hsh.svl : -MATE_VALUE);
		} else if (hsh.ucFlag == HASH_ALPHA) {
			return (hsh.svl <= alpha ? hsh.svl : -MATE_VALUE);
		}else return hsh.svl;
	}
	return -MATE_VALUE;
}

void RecordHash(int nFlag, int value, int nDepth, int mv) {
	HashStruct hsh;
	hsh = SEARCH.hashTable[POS.getZobr().getKey() & (HASH_SIZE-1)];
	if (hsh.ucDepth > nDepth) return;
	hsh.ucFlag = nFlag;
	hsh.ucDepth = nDepth;
	if (value > WIN_VALUE) {
		if (mv == 0 && value <= BAN_VALUE) return;
		hsh.svl = value + POS.get_nDistance();
	} else if (value < -WIN_VALUE) {
		if (mv == 0 && value >= -BAN_VALUE) return;
		hsh.svl = value - POS.get_nDistance();
	} else hsh.svl = value;
	hsh.wmv = mv;
	hsh.lock0 = POS.getZobr().getLock(0);
	hsh.lock1 = POS.getZobr().getLock(1);
	SEARCH.hashTable[POS.getZobr().getKey() & (HASH_SIZE-1)] = hsh;
}

bool CompareMvvLva(const int &mv1, const int &mv2) {
	return Mvvlva(mv2) < Mvvlva(mv1);
}

bool CompareHistory(const int &h1, const int &h2) {
	return SEARCH.nHistoryTable[h2] < SEARCH.nHistoryTable[h1];
}

void MoveSort::init(int _mvhsh) {
	mvHash = _mvhsh;
	for (int i=0; i<2; ++i)
		mvKiller[i] = SEARCH.mvKillers[POS.get_nDistance()][i];
	nPhase = PHASE_HASH;
}

int MoveSort::getNext() {
	int mv;
	switch (nPhase) {
	case PHASE_HASH:
		nPhase = PHASE_KILLER0;
		if (mvHash != 0) return mvHash;
	
	case PHASE_KILLER0:
		nPhase = PHASE_KILLER1;
		if (mvKiller[0] != mvHash && mvKiller[0] != 0 && POS.isLegalMove(mvKiller[0]))
			return mvKiller[0];
	case PHASE_KILLER1:
		nPhase = PHASE_GEN_MOVES;
		if (mvKiller[1] != mvHash && mvKiller[1] != 0 && POS.isLegalMove(mvKiller[1]))
			return mvKiller[1];
	case PHASE_GEN_MOVES:
		nPhase = PHASE_REST;
		nGenMoves = POS.genMoves(mvs);
		std::sort(mvs,mvs+nGenMoves,CompareHistory);
		nIndex = 0;
	case PHASE_REST:
		while(nIndex < nGenMoves) {
			mv = mvs[nIndex++];
			if (mv != mvHash && mv != mvKiller[0] && mv != mvKiller[1]) return mv;
		}
	default: return 0;
	}
}

inline void SetBestMove(int mv, int nDepth) {
	int *pmvKillers;
	SEARCH.nHistoryTable[mv] += nDepth*nDepth;
	pmvKillers = SEARCH.mvKillers[POS.get_nDistance()];
	if (pmvKillers[0] != mv) {
		pmvKillers[1] = pmvKillers[0];
		pmvKillers[0] = mv;
	}
}

static int SearchQuiesc(int alpha, int beta) {
	int i, nGenMoves;
	int value, valueBst;
	int mvs[MAX_GEN_MOVES];


	value = POS.repStatus();
	if (value != 0) {
		return POS.repValue(value);
	}

	if (POS.get_nDistance() == LIMIT_DEPTH) {
		return POS.evaluate();
	}

	valueBst = -MATE_VALUE;
	if (POS.isLastInCheck()) {
		nGenMoves = POS.genMoves(mvs);
		std::sort(mvs, mvs + nGenMoves, CompareHistory);
	} else {

		value = POS.evaluate();
		if (value > valueBst) {
			valueBst = value;
			if (value >= beta) return value;
			if (value > alpha) alpha = value;
		}

		nGenMoves = POS.genMoves(mvs, GEN_CAPTURE);
		std::sort(mvs, mvs + nGenMoves, CompareMvvLva);
	}

	for (i=0; i<nGenMoves; ++i) {
		if (POS.makeMove(mvs[i])) {
			value = -SearchQuiesc(-beta, -alpha);
			POS.undoMakeMove();

			if (value > valueBst) {
				valueBst = value;
				if (value >= beta) return value;
				if (value > alpha) alpha = value;
			}
		}
	}

	return valueBst == -MATE_VALUE ? POS.get_nDistance() - MATE_VALUE : valueBst;
}

int SearchFull(int alpha, int beta, int nDepth, bool isNotNull) {
	int nHashFlag, value, valueBst;
	int mv, mvBst, mvHash, nNewDepth;
	MoveSort msort;

	if (nDepth <= 0) {
		return SearchQuiesc(alpha, beta);
	}

	value =  POS.repStatus();
	if (value != 0) return POS.repValue(value);

	if (POS.get_nDistance() == LIMIT_DEPTH)
		return POS.evaluate();

	value = ProbeHash(alpha, beta, nDepth, mvHash);
	if (value > -MATE_VALUE) return value;

	if (!isNotNull && !POS.isLastInCheck() && POS.isNullOkay()) {
		POS.makeNullMove();
		value = -SearchFull(-beta, 1-beta, nDepth-NULL_DEPTH-1, NO_NULL);
		POS.undoMakeNullMove();
		if (value >= beta) return value;
	}

	nHashFlag = HASH_ALPHA;
	valueBst = -MATE_VALUE;
	mvBst = 0;

	msort.init(mvHash);
	while((mv = msort.getNext()) != 0) {
		if (POS.makeMove(mv)) {
			nNewDepth = POS.isLastInCheck() ? nDepth : nDepth - 1;

			//PVS
			if (valueBst == -MATE_VALUE) {
				value = -SearchFull(-beta, -alpha, nNewDepth);
			} else {
				value = -SearchFull(-alpha-1, -alpha, nNewDepth);
				if (value > alpha && value < beta) {
					value = -SearchFull(-beta, -alpha, nNewDepth);
				}
			}
			POS.undoMakeMove();

			if (value > valueBst) {
				valueBst = value;
				if (value >= beta) {
					nHashFlag = HASH_BETA;
					mvBst = mv;
					break;
				}
				if (value > alpha) {
					nHashFlag = HASH_PV;
					mvBst = mv;
					alpha = value;
				}
			}
		}
	}

	if (valueBst == -MATE_VALUE) 
		return POS.get_nDistance()-MATE_VALUE;
	RecordHash(nHashFlag, valueBst, nDepth, mvBst);
	if (mvBst != 0) SetBestMove(mvBst, nDepth);
	return valueBst;
}

int SearchRoot (int nDepth) {
	int value, valueBst, mv, nNewDepth;
	MoveSort msort;

	valueBst = -MATE_VALUE;
	msort.init(SEARCH.mvResult);
	while ((mv = msort.getNext()) != 0) {
		if (POS.makeMove(mv)) {
			nNewDepth = POS.isLastInCheck() ? nDepth : nDepth-1;
			if (valueBst == -MATE_VALUE) {
				value = -SearchFull(-MATE_VALUE, MATE_VALUE, nNewDepth, NO_NULL);
			} else {
				value = -SearchFull(-valueBst-1, -valueBst, nNewDepth);
				if (value > valueBst) {
					value = -SearchFull(-MATE_VALUE, -valueBst, nNewDepth, NO_NULL);
				}
			}
			POS.undoMakeMove();
			if (value > valueBst) {
				valueBst = value;
				SEARCH.mvResult = mv;
				if (valueBst > -WIN_VALUE && valueBst < WIN_VALUE)
					valueBst += (rand() & RANDOM_MASK) - (rand() & RANDOM_MASK);
			}
		}
	}
	RecordHash(HASH_PV, valueBst, nDepth, SEARCH.mvResult);
	SetBestMove(SEARCH.mvResult, nDepth);
	return valueBst;
}

void SearchMain() {
	int i,t,value, nGenMoves;
	int mvs[MAX_GEN_MOVES];

	memset(SEARCH.nHistoryTable,0,sizeof(SEARCH.nHistoryTable));
	memset(SEARCH.mvKillers,0,sizeof(SEARCH.mvKillers));
	memset(SEARCH.hashTable,0,sizeof(SEARCH.hashTable));

	t = clock();
	POS.set_nDistance(0);


	SEARCH.mvResult = SearchBook();
	if (SEARCH.mvResult != 0) {
		POS.makeMove(SEARCH.mvResult);
		if (POS.repStatus(3) == 0) {
			POS.undoMakeMove();
			return;
		}
		POS.undoMakeMove();
	}

	value = 0;
	nGenMoves = POS.genMoves(mvs);
	for (i=0; i<nGenMoves; ++i) {
		if (POS.makeMove(mvs[i])) {
			POS.undoMakeMove();
			SEARCH.mvResult = mvs[i];
			++value;
		}
	}
	if (value == 1) return;

	for (i=1; i<=LIMIT_DEPTH; ++i) {
		value = SearchRoot(i);

		if (value > WIN_VALUE || value < -WIN_VALUE) break;
		double timeLimit = POS.getMoveNum() > 10 ? CLOCKS_PER_SEC : 0.5*CLOCKS_PER_SEC;
		if (clock() - t > timeLimit) break;
	}
}
static BYTE cucMvvlva[24] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	5, 1, 1, 3, 4, 3, 2, 0,
	5, 1, 1, 3, 4, 3, 2, 0
};

inline int Mvvlva (int mv) {
	return (cucMvvlva[POS.getSquareAt(getMoveDst(mv)) << 3]) - 
		cucMvvlva[POS.getSquareAt(getMoveSrc(mv))];
}