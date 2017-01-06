#include "HELPER.H"
#include "BOARD.H"
#include <cstring>
#include <windows.h>

void Position::clearBoard() {
	sdPlayer = valueRed = valueBlack = nDistance = 0;
	memset(ucpcSquares,0,sizeof(ucpcSquares));
	zobr.clear();
}

void Position::clearHist() {
	mvsList[0].set(0,0,isChecked(),zobr.getKey());
	nMoveNum = 1;
}

void Position::startUp() {
	int pos, pc;
	clearBoard();
	for (pos = 0; pos<256; ++pos) {
		pc = cucpcStartup[pos];
		if (pc != 0) addPiece(pos,pc);
	}
	clearHist();
}

void Position::changeSide() {
	sdPlayer = 1 - sdPlayer;
	zobr.xor(ZOBRIST.player);
}

void Position::addPiece(int pos, int pc) {
	ucpcSquares[pos] = pc;
	if (pc < 16){
		valueRed += cucvlPiecePos[pc-8][pos];
		zobr.xor(ZOBRIST.table[pc-8][pos]);
	} else {
		valueBlack += cucvlPiecePos[pc-16][flipSquare(pos)];
		zobr.xor(ZOBRIST.table[pc-9][pos]);
	}
}

void Position::delPiece(int pos, int pc) {
	ucpcSquares[pos] = 0;
	if (pc < 16){
		valueRed -= cucvlPiecePos[pc-8][pos];
		zobr.xor(ZOBRIST.table[pc-8][pos]);
	} else {
		valueBlack -= cucvlPiecePos[pc-16][flipSquare(pos)];
		zobr.xor(ZOBRIST.table[pc-9][pos]);
	}
}

int Position::evaluate() const {
	return (sdPlayer == 0 ? valueRed - valueBlack : valueBlack - valueRed) + ADVANCED_VALUE;
}

bool Position::isLastInCheck() const {
	return mvsList[nMoveNum-1].isCheck();
}

bool Position::isLastCaptured() const {
	return mvsList[nMoveNum-1].isCaptured();
}

int Position::movePiece(int mv) {
	int src, dst, pc,pcCap;
	src = getMoveSrc(mv);
	dst = getMoveDst(mv);
	pcCap = ucpcSquares[dst];
	if (pcCap != 0) delPiece(dst, pcCap);
	pc = ucpcSquares[src];
	delPiece(src,pc);
	addPiece(dst,pc);
	return pcCap;
}

void Position::undoMovePiece(int mv, int pcCaptured) {
	int src, dst, pc;
	src = getMoveSrc(mv);
	dst = getMoveDst(mv);
	pc = ucpcSquares[dst];
	delPiece(dst,pc);
	addPiece(src,pc);
	if (pcCaptured != 0) {
		addPiece(dst,pcCaptured);
	}
}

bool Position::makeMove(int mv) {
	int pcCap;
	UL k = zobr.getKey();
	pcCap = movePiece(mv);
	if (isChecked()) {
		undoMovePiece(mv,pcCap);
		return false;
	}
	changeSide();
	mvsList[nMoveNum].set(mv,pcCap,isChecked(),k);
	++nMoveNum;
	++nDistance;
	return true;
}

void Position::undoMakeMove() {
	--nDistance;
	--nMoveNum;
	changeSide();
	undoMovePiece(mvsList[nMoveNum].getMv(),mvsList[nMoveNum].getCapturedOne());
}

void Position::makeNullMove() {
	UL k = zobr.getKey();
	changeSide();
	mvsList[nMoveNum].set(0,0,false,k);
	++nMoveNum;
	++nDistance;
}

void Position::undoMakeNullMove() {
	--nDistance;
	--nMoveNum;
	changeSide();
}

bool Position::isMate() {
	// return false;
	int i,nGenMoveNum, pcCap;
	int mvs[MAX_GEN_MOVES];
	nGenMoveNum = genMoves(mvs);
	for (i=0; i<nGenMoveNum; ++i) {
		pcCap = movePiece(mvs[i]);
		if (!isChecked()) {
			undoMovePiece(mvs[i],pcCap);
			return false;
		} else {
			undoMovePiece(mvs[i],pcCap);
		}
	}
	return true;
}

int Position::genMoves(int *mvs, bool isCapture) const {

	int i, j, nGenMoves, nDelta, sqSrc, sqDst;
	int pcSelfSide, pcOppSide, pcSrc, pcDst;

	nGenMoves = 0;
	pcSelfSide = getSideTag(sdPlayer);
	pcOppSide = getOppSideTag(sdPlayer);
	for (sqSrc = 0; sqSrc < 256; ++sqSrc) {

	pcSrc = ucpcSquares[sqSrc];
	if ((pcSrc & pcSelfSide) == 0) continue;

	switch (pcSrc - pcSelfSide) {
	case PIECE_KING:
		for (i = 0; i < 4; i ++) {
			sqDst = sqSrc + ccKingDelta[i];
			if (!isInFort(sqDst)) continue;
			pcDst = ucpcSquares[sqDst];
			if (isCapture ? (pcDst & pcOppSide) != 0 : (pcDst & pcSelfSide) == 0) {
			  mvs[nGenMoves++] = getMove(sqSrc, sqDst);
			}
		}
		break;
	case PIECE_ADVISOR:
		for (i = 0; i < 4; i ++) {
			sqDst = sqSrc + ccAdvisorDelta[i];
			if (!isInFort(sqDst)) continue;
			pcDst = ucpcSquares[sqDst];
			if (isCapture ? (pcDst & pcOppSide) != 0 : (pcDst & pcSelfSide) == 0) {
				mvs[nGenMoves++] = getMove(sqSrc, sqDst);
			}
		}
		break;
	case PIECE_BISHOP:
		for (i = 0; i < 4; i ++) {
			sqDst = sqSrc + ccAdvisorDelta[i];
			if (!(isInBoard(sqDst) && isHomeHalf(sqDst, sdPlayer) && ucpcSquares[sqDst] == 0))
				continue;
			sqDst += ccAdvisorDelta[i];
			pcDst = ucpcSquares[sqDst];
			if (isCapture ? (pcDst & pcOppSide) != 0 : (pcDst & pcSelfSide) == 0) {
				mvs[nGenMoves++] = getMove(sqSrc, sqDst);
			}
		}
		break;
	case PIECE_KNIGHT:
		for (i = 0; i < 4; i ++) {
			sqDst = sqSrc + ccKingDelta[i];
			if (ucpcSquares[sqDst] != 0) continue;
			for (j = 0; j < 2; j ++) {
				sqDst = sqSrc + ccKnightDelta[i][j];
				if (!isInBoard(sqDst)) continue;
				pcDst = ucpcSquares[sqDst];
				if (isCapture ? (pcDst & pcOppSide) != 0 : (pcDst & pcSelfSide) == 0) {
					mvs[nGenMoves++] = getMove(sqSrc, sqDst);
				}
			}
		}
		break;
	case PIECE_ROOK:
		for (i = 0; i < 4; i ++) {
			nDelta = ccKingDelta[i];
			sqDst = sqSrc + nDelta;
			while (isInBoard(sqDst)) {
				pcDst = ucpcSquares[sqDst];
				if (pcDst == 0) {
					if (!isCapture) {
						mvs[nGenMoves++] = getMove(sqSrc, sqDst);
					}
				} else {
					if ((pcDst & pcOppSide) != 0) {
						mvs[nGenMoves++] = getMove(sqSrc, sqDst);
					}
					break;
				}
				sqDst += nDelta;
			}
		}
		break;
	case PIECE_CANNON:
		for (i = 0; i < 4; i ++) {
			nDelta = ccKingDelta[i];
			sqDst = sqSrc + nDelta;
			while (isInBoard(sqDst)) {
				pcDst = ucpcSquares[sqDst];
				if (pcDst == 0) {
					if (!isCapture) {
						mvs[nGenMoves++] = getMove(sqSrc, sqDst);
					}
				} else {
					break;
				}
				sqDst += nDelta;
			}
			sqDst += nDelta;
			while (isInBoard(sqDst)) {
				pcDst = ucpcSquares[sqDst];
				if (pcDst != 0) {
					if ((pcDst & pcOppSide) != 0) {
						mvs[nGenMoves++] = getMove(sqSrc, sqDst);
					}
					break;
				}
				sqDst += nDelta;
			}
		}
		break;
	case PIECE_PAWN:
		sqDst = forwardSquare(sqSrc, sdPlayer);
		if (isInBoard(sqDst)) {
			pcDst = ucpcSquares[sqDst];
			if (isCapture ? (pcDst & pcOppSide) != 0 : (pcDst & pcSelfSide) == 0) {
				mvs[nGenMoves++] = getMove(sqSrc, sqDst);
			}
		}
		if (isAwayHalf(sqSrc, sdPlayer)) {
			for (nDelta = -1; nDelta <= 1; nDelta += 2) {
				sqDst = sqSrc + nDelta;
				if (isInBoard(sqDst)) {
					pcDst = ucpcSquares[sqDst];
					if (isCapture ? (pcDst & pcOppSide) != 0 : (pcDst & pcSelfSide) == 0) {
				  		mvs[nGenMoves++] = getMove(sqSrc, sqDst);
					}
				}
			}
		}
		break;
	}
	}
	return nGenMoves;
}

bool Position::isLegalMove(int mv) const {
	int sqSrc, sqDst, sqPin;
	int pcSelfSide, pcSrc, pcDst, nDelta;

	sqSrc = getMoveSrc(mv);
	pcSrc = ucpcSquares[sqSrc];
	pcSelfSide = getSideTag(sdPlayer);
	if ((pcSrc & pcSelfSide) == 0) return false;

	sqDst = getMoveDst(mv);
	pcDst = ucpcSquares[sqDst];
	if ((pcDst & pcSelfSide) != 0) return false;

	switch(pcSrc-pcSelfSide) {
	case PIECE_KING:
		return isInFort(sqDst) && kingSpan(sqSrc, sqDst);
	case PIECE_ADVISOR:
		return isInFort(sqDst) && advisorSpan(sqSrc, sqDst);
	case PIECE_BISHOP:
		return isSameHalf(sqSrc, sqDst) && bishopSpan(sqSrc, sqDst) &&
			ucpcSquares[bishopPin(sqSrc, sqDst)] == 0;
	case PIECE_KNIGHT:
		sqPin = knightPin(sqSrc, sqDst);
		return sqSrc != sqPin && ucpcSquares[sqPin] == 0;
	case PIECE_ROOK:
	case PIECE_CANNON:
		if (isSameRank(sqSrc, sqDst)) {
			nDelta = sqDst < sqSrc ? -1 : 1;
		} else if (isSameFile(sqSrc, sqDst)) {
			nDelta = sqDst < sqSrc ? -16 : 16;
		} else return false;

		sqPin = sqSrc + nDelta;
		while(sqPin != sqDst && ucpcSquares[sqPin] == 0) sqPin += nDelta;
		if (sqDst == sqPin) return pcDst == 0 || pcSrc-pcSelfSide == PIECE_ROOK;
		else if (pcDst != 0 && pcSrc - pcSelfSide == PIECE_CANNON) {
			sqPin += nDelta;
			while(sqPin != sqDst && ucpcSquares[sqPin] == 0) sqPin += nDelta;
			return sqPin == sqDst;
		} else return false;
	case PIECE_PAWN:
		if (isAwayHalf(sqDst, sdPlayer) && (sqDst == sqSrc+1 || sqDst == sqSrc-1)) {
			return true;
		}
		return sqDst == forwardSquare(sqSrc, sdPlayer);
	default:
		return false;
	}
}

bool Position::isChecked() const {
	int i,j,sqSrc,sqDst;
	int pcSelfSide, pcOppSide, pcDst, nDelta;
	pcSelfSide = getSideTag(sdPlayer);
	pcOppSide = getOppSideTag(sdPlayer);

	for (sqSrc = 0; sqSrc < 256; ++sqSrc) {
		if (ucpcSquares[sqSrc] != pcSelfSide + PIECE_KING) continue;


		if (ucpcSquares[forwardSquare(sqSrc, sdPlayer)] == pcOppSide + PIECE_PAWN)
			return true;
		for (nDelta = -1; nDelta <=1; nDelta += 2) {
			if (ucpcSquares[sqSrc + nDelta] == pcOppSide + PIECE_PAWN)
				return true;
		}


		for (i=0; i<4; ++i) {
			if (ucpcSquares[sqSrc + ccAdvisorDelta[i]] != 0) continue;
			for (j=0; j<2; ++j) {
				pcDst = ucpcSquares[sqSrc + ccKnightCheckDelta[i][j]];
				if (pcDst == pcOppSide + PIECE_KNIGHT) return true;
			}
		}


		for (i=0; i<4; ++i) {
			nDelta = ccKingDelta[i];
			sqDst = sqSrc + nDelta;
			while(isInBoard(sqDst)) {
				pcDst = ucpcSquares[sqDst];
				if (pcDst != 0) {
					if (pcDst == pcOppSide + PIECE_ROOK || pcDst ==  pcOppSide + PIECE_KING)
						return true;
					break;
				}
				sqDst += nDelta;
			}
			sqDst += nDelta;
			while(isInBoard(sqDst)) {
				pcDst = ucpcSquares[sqDst];
				if (pcDst != 0) {
					if (pcDst == pcOppSide + PIECE_CANNON) return true;
				}
				break;
			}
		}
		return false;
	}
	return false;
}


int Position::drawValue() const {
	return (nDistance&1)==0?-DRAW_VALUE:DRAW_VALUE;
}

int Position::repStatus(int nRecur) const {
	bool isSelfSide, isPerpCheck, isOppPerpCheck;
	const Move *lpmvs;

	isSelfSide = false;
	isPerpCheck = isOppPerpCheck = true;
	lpmvs = mvsList + nMoveNum - 1;
	while (lpmvs->getMv() != 0 && lpmvs->getCapturedOne() == 0) {
		if (isSelfSide) {
			isPerpCheck = isPerpCheck && lpmvs->isCheck();
			if (lpmvs->getKey() == zobr.getKey()) {
				--nRecur;
				if (nRecur == 0) return 1+(isPerpCheck?2:0) + (isOppPerpCheck?4:0);
			}
		} else {
			isOppPerpCheck = isOppPerpCheck && lpmvs->isCheck();
		}
		isSelfSide = !isSelfSide;
		lpmvs--;
	}
	return 0;
}

int Position::repValue(int nRepStatus) const {
	int ret;
	ret = ((nRepStatus & 2) == 0 ? 0:nDistance-BAN_VALUE) + 
		((nRepStatus & 4) == 0 ? 0:BAN_VALUE-nDistance);
	return ret == 0 ? drawValue() : ret; 
}

bool Position::isNullOkay() const {
	return (sdPlayer == 0 ? valueRed : valueBlack) > NULL_MARGIN;
}

void Position::mirror(Position &posMirror) const {
	int sq,pc;
	posMirror.clearBoard();
	for (sq = 0; sq < 256; ++sq) {
		pc = ucpcSquares[sq];
		if (pc != 0) posMirror.addPiece(mirrorSquare(sq),pc);
	}
	if (sdPlayer == 1) {
		posMirror.changeSide();
	}
	posMirror.clearHist();
}

