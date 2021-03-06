#ifndef NAIVEBOARD_H
#define NAIVEBOARD_H

#include "board/board.h"
#include "util/def.h"
#include "util/co.h"
#include "util/hash.h"

#include <vector>
#include <unordered_set>
#include <memory>
#include <stack>

using def::PLAYER_E;
using def::TPos;
using def::TDelta;
using def::TMove;
using std::vector;
using std::unordered_set;
using std::shared_ptr;
using std::stack;

class NaiveBoard : public board::IBoard
{
public:    
    NaiveBoard();

    virtual void init();                                    // 开局
    virtual uint8_t autoMove();                             // 电脑走棋,返回EMoveRet的组合
    virtual uint8_t makeMove(def::TMove move);              // 指定走法走棋,返回EMoveRet的组合
    virtual bool undoMakeMove();                            // 悔棋

    virtual int getScore(def::PLAYER_E player) const;        // 获取当前局面下的玩家分数
    virtual def::ICON_E getIcon(def::TPos pos) const;      // 获取某一位置的棋子
    virtual def::PLAYER_E getNextPlayer() const;             // 获取下一走棋玩家
    virtual def::PLAYER_E getOwner(def::TPos pos) const;// 获取pos棋子所属玩家
    virtual def::TMove getTrigger() const;                  // 表示该snapshot是由trigger的两个位置移动产生的，用于绘制select图标

protected:
    // 玩家棋子位置集合
    struct TPieceSet
    {
        TPos king;
        int score;
        unordered_set<TPos> defenders;
        unordered_set<TPos> attackers;

        TPieceSet()
            : king(def::INVALID_POS)
            , score(0)
        {

        }

        void swap(TPieceSet& other)
        {
            std::swap(king, other.king);
            std::swap(score, other.score);
            defenders.swap(other.defenders);
            attackers.swap(other.attackers);
        }
    };

    // 当前局面快照
    class Snapshot
    {
        friend class NaiveBoard;

        vector<vector<uint8_t>> board_;// 棋盘
        PLAYER_E player_;// 下一步要走棋的玩家

        TPieceSet upPieceSet_;// 上方玩家棋子集合
        TPieceSet downPieceSet_;// 下方玩家棋子集合

        // 第四位和第五位表示上下部分
        uint8_t blackFlag_;// 默认上面为黑色
        uint8_t redFlag_;// 默认下面为红色

        def::TMove trigger_;// 表示该snapshot是由trigger的两个位置移动产生的，用于绘制select图标

    public:
        Snapshot()
            : trigger_(def::INVALID_POS, def::INVALID_POS)
        {
            static const vector<vector<uint8_t>> initialBoard = {
                     /* edge            mid            edge */
                     /*  0   1   2   3   4   5   6   7   8  */
              /* 0 */ { 21, 20, 19, 18, 17, 18, 19, 20, 21 }, /*  edge */
              /* 1 */ {  0,  0,  0,  0,  0,  0,  0,  0,  0 },
              /* 2 */ {  0, 22,  0,  0,  0,  0,  0, 22,  0 }, /*  up   */
              /* 3 */ { 23,  0, 23,  0, 23,  0, 23,  0, 23 },
              /* 4 */ {  0,  0,  0,  0,  0,  0,  0,  0,  0 }, /*  mid  */
              /* 5 */ {  0,  0,  0,  0,  0,  0,  0,  0,  0 }, /*  mid  */
              /* 6 */ { 15,  0, 15,  0, 15,  0, 15,  0, 15 },
              /* 7 */ {  0, 14,  0,  0,  0,  0,  0, 14,  0 }, /*  down */
              /* 8 */ {  0,  0,  0,  0,  0,  0,  0,  0,  0 },
              /* 9 */ { 13, 12, 11, 10,  9, 10, 11, 12, 13 }, /*  edge */
            };

            // 初始化snapshot
            board_ = initialBoard;
            player_ = def::PLAYER_red;// 下方先走
            // 上方棋子集合
            upPieceSet_.king = {0, 4};
            upPieceSet_.score = 888;
            upPieceSet_.defenders =
            {
                {0, 2},
                {0, 3},
                {0, 5},
                {0, 6},
            };
            upPieceSet_.attackers =
            {
                {0, 0},
                {0, 1},
                {0, 7},
                {0, 8},
                {2, 1},
                {2, 7},
                {3, 0},
                {3, 2},
                {3, 4},
                {3, 6},
                {3, 8},
            };
            // 下方棋子集合
            downPieceSet_.king = {9, 4};
            downPieceSet_.score = 888;
            downPieceSet_.defenders =
            {
                {9, 2},
                {9, 3},
                {9, 5},
                {9, 6},
            };
            downPieceSet_.attackers =
            {
                {9, 0},
                {9, 1},
                {9, 7},
                {9, 8},
                {7, 1},
                {7, 7},
                {6, 0},
                {6, 2},
                {6, 4},
                {6, 6},
                {6, 8},
            };

            blackFlag_ = def::PLAYER_black; // 默认上面为黑色
            redFlag_   = def::PLAYER_red;   // 默认下面为红色
        }

        Snapshot(const Snapshot& other)
            : trigger_(other.trigger_)
        {
            board_ = other.board_;
            player_ = other.player_;
            upPieceSet_ = other.upPieceSet_;
            downPieceSet_ = other.downPieceSet_;
            blackFlag_ = other.blackFlag_;
            redFlag_ = other.redFlag_;
            // trigger_ = other.trigger_;
        }

        // 赋值运算符的现代写法
        Snapshot& operator=(Snapshot other)
        {
            swap(other);
            return *this;
        }

        void swap(Snapshot& other)
        {
            board_.swap(other.board_);
            std::swap(player_,other.player_);
            upPieceSet_.swap(other.upPieceSet_);
            downPieceSet_.swap(other.downPieceSet_);
            std::swap(blackFlag_, other.blackFlag_);
            std::swap(redFlag_, other.redFlag_);
            std::swap(trigger_, other.trigger_);
        }

        /*void rotate()
        {
            //todo: optimize
            vector<vector<uint8_t>> tmp(co::g_rowNum, vector<uint8_t>(co::g_colNum, 0));
            for (int i = 0; i < co::g_rowNum; i++)
            {
                for (int j = 0; j < co::g_colNum; j++)
                {
                    TPos rotatePos = co::getRotatePos({i, j});
                    tmp[i][j] = board_[rotatePos.row][rotatePos.col];
                }
            }

            board_.swap(tmp);
            def::switchPlayer(player_);
            upPieceSet_.swap(downPieceSet_);
            std::swap(blackFlag_, redFlag_);// 交换上下方标志位
            trigger_.first = co::getRotatePos(trigger_.first);// 旋转trigger
            trigger_.second = co::getRotatePos(trigger_.second);
        }*/
    };

    // 对于player来说，从prevPos到currPos是否合法
    bool isValidMove(TMove move, PLAYER_E player) const;

    typedef bool (NaiveBoard::*PosFunc)(TPos pos, PLAYER_E player) const;// 判断位置是否合法
    typedef bool (NaiveBoard::*DeltaFunc)(TDelta delta, PLAYER_E player) const;// 判断偏移量是否合法
    typedef bool (NaiveBoard::*RuleFunc)(TMove move) const;// 棋子特定的规则
    bool isValidMove(TMove move, PLAYER_E player, PosFunc isValidPos, DeltaFunc isValidDelta, RuleFunc isValidRule) const;

    bool isValidKingPos(TPos pos, PLAYER_E player) const;
    bool isValidAdvisorPos(TPos pos, PLAYER_E player) const;
    bool isValidBishopPos(TPos pos,def:: PLAYER_E player) const;
    bool isValidKnightPos(TPos pos, PLAYER_E player) const;
    bool isValidRookPos(TPos pos, PLAYER_E player) const;
    bool isValidCannonPos(TPos pos, PLAYER_E player) const;
    bool isValidPawnPos(TPos pos, PLAYER_E player) const;

    bool isValidKingDelta(TDelta delta, PLAYER_E player) const;
    bool isValidAdvisorDelta(TDelta delta, PLAYER_E player) const;
    bool isValidBishopDelta(TDelta delta, PLAYER_E player) const;
    bool isValidKnightDelta(TDelta delta, PLAYER_E player) const;
    bool isValidRookDelta(TDelta delta, PLAYER_E player) const;
    bool isValidCannonDelta(TDelta delta, PLAYER_E player) const;
    bool isValidPawnDelta(TDelta delta, PLAYER_E player) const;

    // 不再检查pos及delta，默认前面已检查
    bool isValidKingRule(TMove move) const;
    bool isValidAdvisorRule(TMove move) const;
    bool isValidBishopRule(TMove move) const;
    bool isValidKnightRule(TMove move) const;
    bool isValidRookRule(TMove move) const;
    bool isValidCannonRule(TMove move) const;
    bool isValidPawnRule(TMove move) const;

    bool isValidKingMove(TMove move, PLAYER_E player) const;
    bool isValidAdvisorMove(TMove move, PLAYER_E player) const;
    bool isValidBishopMove(TMove move, PLAYER_E player) const;
    bool isValidKnightMove(TMove move, PLAYER_E player) const;
    bool isValidRookMove(TMove move, PLAYER_E player) const;
    bool isValidCannonMove(TMove move, PLAYER_E player) const;
    bool isValidPawnMove(TMove move, PLAYER_E player) const;

    const unordered_set<TDelta>& getValidKingDelta(PLAYER_E player) const;
    const unordered_set<TDelta>& getValidAdvisorDelta(PLAYER_E player) const;
    const unordered_set<TDelta>& getValidBishopDelta(PLAYER_E player) const;
    const unordered_set<TDelta>& getValidKnightDelta(PLAYER_E player) const;
    const unordered_set<TDelta>& getValidRookDelta(PLAYER_E player) const;
    const unordered_set<TDelta>& getValidCannonDelta(PLAYER_E player) const;
    const unordered_set<TDelta>& getValidPawnDelta(PLAYER_E player) const;

    int getValue(uint8_t piece, TPos pos, PLAYER_E player) const;

    bool check(PLAYER_E player);
    bool checkmate(PLAYER_E player);

    bool isSuicide(TMove move, PLAYER_E player);
    bool updatePieceSet(TMove move, PLAYER_E player);

    inline bool isPiece(TPos pos, int piece) const;// 判断pos位置是否为特定棋子（不分颜色）
    inline bool isKing(TPos pos) const;
    inline bool isDefensivePiece(TPos pos) const;
    inline bool isAttactivePiece(TPos pos) const;

    bool isKingMeeting() const;

    shared_ptr<Snapshot> createSnapshot();
    bool saveSnapshot();
    bool loadSnapshot();
    bool updateSnapshot(TMove move, PLAYER_E player);

    void generateAllMoves(vector<def::TMove>& moves);// 生成当前局面所有合法走法

    int minimax(int depth, def::PLAYER_E maxPlayer, TMove& move);
    int alphabeta(int depth, def::PLAYER_E maxPlayer, int alpha, int beta, TMove& move);

private:
    shared_ptr<Snapshot> snapshot_;
    stack<shared_ptr<Snapshot>> snapshotMemo_;
};

#endif // NAIVEBOARD_H
