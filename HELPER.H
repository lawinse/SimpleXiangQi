#ifndef HELPER_H
#define HELPER_H
#include <stdint.h>
#include "BASE.H"
#include <algorithm>
typedef DWORD UL;
typedef unsigned short US;

class RC4 {
	BYTE s[256];
	int x,y;
public:
	void clear() {
		int i,j;
		x = y = j = 0;
		for (i=0; i<256; ++i) {
			s[i] = i;
		}
		for (i=0; i<256; ++i) {
			j = (j+s[i]) & 255;
			std::swap(s[i],s[j]);
		}
	}

	BYTE nextByte() {
		x = (x+1)&255;
		y = (y+s[x])&255;
		std::swap(s[x],s[y]);
		return s[(s[x]+s[y])&255];
	}

	UL nextLong() {
		return nextByte() + (nextByte() << 8) + (nextByte() << 16) + (nextByte() << 24);
	}
};

class Zobrist {
	UL key, lock0, lock1;
public:
	UL getLock(bool which) const{return which?lock1:lock0;}
	UL getKey() const {return key;}
	void clear() {
		key = lock1 = lock0 = 0;
	}

	void initByRC4(RC4 &rc4) {
		key = rc4.nextLong();
		lock0 = rc4.nextLong();
		lock1 = rc4.nextLong();
	}

	void xor(const Zobrist &zbr) {
		key ^= zbr.key;
		lock1 ^= zbr.lock1;
		lock0 ^= zbr.lock0;
	}

	void xor(const Zobrist &z1, const Zobrist &z2) {
		xor(z1);
		xor(z2);
	}
};

struct ZobStruct{
	Zobrist player;
	Zobrist table[14][256];
};

extern ZobStruct ZOBRIST;

static void initZOBRIST() {
	int i,j;
	RC4 rc4;
	rc4.clear();
	ZOBRIST.player.initByRC4(rc4);
	for (i=0; i<14; ++i)
		for (j=0; j<256; ++j)
			ZOBRIST.table[i][j].initByRC4(rc4);
}

class Move {
	int wmv;
	int capturedOne;
	bool bCheck;
	UL key;
public:
	void set(int mv, int _pcCaptured, bool _isCheck, UL _k) {
		wmv = mv;
		capturedOne = _pcCaptured;
		bCheck = _isCheck;
		key = _k;
	}
	bool isCheck() const{return bCheck;}
	bool isCaptured() const {return capturedOne != 0;}
	int getMv() const {return wmv;}
	int getCapturedOne() const {return capturedOne;}
	UL getKey() const {return key;}

};

class Position{
	int sdPlayer;
	BYTE ucpcSquares[256];
	int valueRed, valueBlack;
	int nDistance, nMoveNum;
	Move mvsList[MAX_MOVES];
	Zobrist zobr;
public:
	int getSdPlayer() const {return sdPlayer;}
	Zobrist getZobr() const {return zobr;}
	int get_nDistance() const {return nDistance;}
	int getMoveNum() const {return nMoveNum;}
	void set_nDistance(int v) {nDistance = v;}
	BYTE getSquareAt(int index) const {return ucpcSquares[index];}
	Move getLastMove() const {return mvsList[nMoveNum-1];}

	void clearBoard();
	void clearHist();
	void startUp();
	void changeSide();
	void addPiece(int pos, int pc);
	void delPiece(int pos, int pc);

	int evaluate() const;
	bool isLastInCheck() const;
	bool isLastCaptured() const;
	int movePiece(int mv);
	void undoMovePiece(int mv, int pcCaptured);
	bool makeMove(int mv);
	int undoMakeMove();
	void makeNullMove();
	void undoMakeNullMove();

	int genMoves(int *mvs, bool isCapture=false) const;
	bool isLegalMove(int mv) const;
	bool isChecked() const;
	bool isMate();
	int drawValue() const;
	int repStatus(int nRecur = 1) const;
	int repValue(int nRepStatus) const;

	bool isNullOkay() const;
	void mirror(Position &posMirror) const;
 };





#endif