#include "palette.h"
#include "chess.h"
#include "resmgr.h"
#include "model/board.h"
#include "util/co.h"
#include "util/debug.h"

#include <QLabel>
#include <QMessageBox>
#include <QSound>
#include <QMediaPlayer>
#include <QCoreApplication>
#include <QMediaPlaylist>

#include <assert.h>
#include <functional>

Palette::Palette(Chess* chess, QLabel* bg, ResMgr* resMgr)
    : soundEffect_(true)
    , chess_(chess)
    , resMgr_(resMgr)
    , bg_(bg)
    , prevPos_(def::g_nullPos)
{
    assert(chess_ != nullptr);   
    assert(bg_ != nullptr);
    assert(resMgr_ != nullptr);    

    board_ = std::make_shared<Board>();    

    initLabel();
    initPieces();    
}

Palette::~Palette()
{

}

// 初始化背景图片、两个选择图标
void Palette::initLabel()
{
    //bg_ = make_shared<QLabel>(chess_);
    bg_->resize(co::g_boardSize.width, co::g_boardSize.height);
    if (shared_ptr<QPixmap> pic = resMgr_->getBg())
    {
        bg_->move(0, 0);
        bg_->setPixmap(*pic.get());
    }

    prevSelect_ = std::make_shared<QLabel>(chess_);
    prevSelect_->resize(co::g_pieceSize.width, co::g_pieceSize.height);
    if (shared_ptr<QPixmap> pic = resMgr_->getPiece(def::EP_select))
        prevSelect_->setPixmap(*pic.get());

    currSelect_ = std::make_shared<QLabel>(chess_);
    currSelect_->resize(co::g_pieceSize.width, co::g_pieceSize.height);
    if (shared_ptr<QPixmap> pic = resMgr_->getPiece(def::EP_select))
        currSelect_->setPixmap(*pic.get());
}

// 每一个位置一个label
void Palette::initPieces()
{
    pieces_.assign(co::g_rowNum, vector<shared_ptr<QLabel>>(co::g_colNum));

    for (int i = 0; i < co::g_rowNum; i++)
    {
        for (int j = 0; j < co::g_colNum; j++)
        {
            pieces_[i][j] = std::make_shared<QLabel>(chess_);
            pieces_[i][j]->resize(co::g_pieceSize.width, co::g_pieceSize.height);
        }
    }
}

void Palette::open()
{
    board_->init();
    drawPieces();

    prevPos_ = def::g_nullPos;
    drawSelect(def::g_nullPos, def::g_nullPos);
}

void Palette::drawBg()
{
    bg_->resize(co::g_boardSize.width, co::g_boardSize.height);
    if (shared_ptr<QPixmap> pic = resMgr_->getBg())
    {
        bg_->move(0, 0);
        bg_->setPixmap(*pic.get());
    }
}

void Palette::drawPieces()
{
    for (int i = 0; i < co::g_rowNum; i++)
    {
        for (int j = 0; j < co::g_colNum; j++)
        {
            drawPiece({i, j});
        }
    }
}

// 将board的pos位置棋子绘制到palette上
void Palette::drawPiece(TPos pos)
{
    if (shared_ptr<QPixmap> pic = resMgr_->getPiece(board_->getPiece(pos)))
        pieces_[pos.row][pos.col]->setPixmap(*pic.get());

    def::TClientCo clientCo = co::pos2ClientCo(pos);
    pieces_[pos.row][pos.col]->move(clientCo.x, clientCo.y);
}

void Palette::drawSelect(TPos prevPos, TPos currPos)
{
    if (co::isValidPos(prevPos))
    {
        prevSelect_->show();

        def::TClientCo clientCo = co::pos2ClientCo(prevPos);
        prevSelect_->move(clientCo.x, clientCo.y);
    }
    else
    {
        prevSelect_->hide();
    }

    if (co::isValidPos(currPos))
    {
        currSelect_->show();

        def::TClientCo clientCo = co::pos2ClientCo(currPos);
        currSelect_->move(clientCo.x, clientCo.y);
    }
    else
    {
        currSelect_->hide();
    }
}

int8 Palette::movePiece(TPos prevPos, TPos currPos)
{
    int8 ret = board_->movePiece(prevPos, currPos);

    if (ret & model::EMR_ok)
    {
        drawPiece(prevPos);
        drawPiece(currPos);
    }

    return ret;
}

void Palette::rotate()
{
    if (board_->rotate())
    {
        drawPieces();
        // 重绘select
        std::pair<TPos, TPos> trigger = board_->getTrigger();
        drawSelect(trigger.first, trigger.second);
    }
}

void Palette::undo()
{
    if (board_->undo())
    {
        drawPieces();
        // 重绘select
        std::pair<TPos, TPos> trigger = board_->getTrigger();
        drawSelect(trigger.first, trigger.second);
    }
}

void Palette::run()
{
    if (board_->run())
    {
        drawPieces();
        // 重绘select
        std::pair<TPos, TPos> trigger = board_->getTrigger();
        drawSelect(trigger.first, trigger.second);
    }
}

void Palette::click(TPos currPos)
{
    if (prevPos_ == def::g_nullPos)
    {
        // 若prevPos_为无效位置，且新位置棋子属于下一步走棋的玩家，则更新prevPos_，并显示当前选择的棋子
        if (board_->getPieceOwner(currPos) == board_->getNextPlayer())
        {
            drawSelect(prevPos_, currPos);
            prevPos_ = currPos;
        }
    }
    else
    {
        // 可以移动棋子
        int8 ret = movePiece(prevPos_, currPos);

        if (soundEffect_)
        {
            if (ret & model::EMR_dead)
                resMgr_->playSound("WIN.WAV");
            else if (ret & model::EMR_suicide)
                resMgr_->playSound("ILLEGAL.WAV");
            else if (ret & model::EMR_check)
                resMgr_->playSound("CHECK.WAV");
            else if (ret & model::EMR_eat)
                resMgr_->playSound("CAPTURE.WAV");
            else if (ret & model::EMR_ok)
                resMgr_->playSound("MOVE.WAV");
        }

        if (ret & model::EMR_ok)
        {
            // 显示两个选择框，prevPos_清空
            drawSelect(prevPos_, currPos);
            prevPos_ = def::g_nullPos;
        }
        else // 不能移动棋子
        {            
            // 当前选择位置与原位置同色，才能更新prevPos_，并绘制选择框
            if (board_->getPieceOwner(prevPos_) == board_->getPieceOwner(currPos))
            {
                prevPos_ = def::g_nullPos;//不标记上次选中的位置
                drawSelect(prevPos_, currPos);
                prevPos_ = currPos;
            }
        }
    }
}

void Palette::soundEffect(bool on)
{
    soundEffect_ = on;    
}

void Palette::bgm(bool on)
{    
    if (on)
        resMgr_->playBgm();
    else
        resMgr_->stopBgm();
}

void Palette::loadBgSkin(ResMgr::EBgSkin skin)
{
    if (resMgr_->loadBgSkin(skin))
    {
        drawBg();
    }
}

void Palette::loadPieceSkin(ResMgr::EPieceSkin skin)
{
    if (resMgr_->loadPieceSkin(skin))
    {
        drawPieces();
    }
}
