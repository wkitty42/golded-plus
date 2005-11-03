
//  ------------------------------------------------------------------
//  GoldED+
//  Copyright (C) 1990-1999 Odinn Sorensen
//  Copyright (C) 1999-2000 Alexander S. Aganichev
//  ------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License as
//  published by the Free Software Foundation; either version 2 of the
//  License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston,
//  MA 02111-1307 USA
//  ------------------------------------------------------------------
//  $Id$
//  ------------------------------------------------------------------
//  Message lister.
//  ------------------------------------------------------------------

#include <golded.h>
#include <gcharset.h>
#include <iostream>
#include <iomanip>

#if defined(__USE_ALLOCA__)
  #include <malloc.h>
#endif


//  ------------------------------------------------------------------

extern GMsg* reader_msg;

static int mlst_bysiz;
static int mlst_tosiz;
static int mlst_resiz;
static int fldadd1;
static int fldadd2;


//  ------------------------------------------------------------------

const byte MLST_HIGH_FROM   =  1;
const byte MLST_HIGH_TO     =  2;
const byte MLST_HIGH_BOOK   =  4;
const byte MLST_HIGH_MARK   =  8;
const byte MLST_HIGH_UNREAD = 16;
const byte MLST_HIGH_UNSENT = 32;


//  ------------------------------------------------------------------

inline void mlst_with_date(int with_date) {

  if(with_date) {
    mlst_bysiz = 19;
    mlst_tosiz = 19;
    mlst_resiz = 20;
  }
  else {
    mlst_bysiz = 19+3;
    mlst_tosiz = 19+3;
    mlst_resiz = 20+4;
  }
}


//  ------------------------------------------------------------------

class GMsgList : public gwinpick {

  gwindow        window;
  GMsg           msg;
  MLst           **mlst;
  uint           msgmark2;

  void open();                        // Called after window is opened
  void close();                       // Called after window is closed
  void update_title();
  void do_delayed();
  void print_line(uint idx, uint pos, bool isbar);
  bool handle_key();                  // Handles keypress
  void update_marks(MLst *ml);
  void ReadMlst(int n);

public:

  void Run();

  GMsgList() {
    memset(&msg, 0, sizeof(GMsg));
    mlst = NULL;
    maximum_index = AA->Msgn.Count()-1;
  };
  ~GMsgList() {
    ResetMsg(&msg);
    if(mlst) {
      for(uint i=0; i<= maximum_index; i++)
        throw_xdelete(mlst[i]);
      throw_free(mlst);
    }
  };
};


//  ------------------------------------------------------------------

void GMsgList::open() {

  window.openxy(ypos, xpos, ylen+2, xlen+2, btype, battr, wattr, sbattr);
  update_title();
  center(CFG->displistcursor);
}


//  ------------------------------------------------------------------

void GMsgList::close() {

  window.close();
}


//  ------------------------------------------------------------------

void GMsgList::update_marks(MLst *ml) {

  ml->high &= ~(MLST_HIGH_BOOK|MLST_HIGH_MARK);

  strcpy(ml->marks, "  ");

  if(AA->bookmark == ml->msgno) {
    ml->marks[0] = MMRK_BOOK;
    ml->high |= MLST_HIGH_BOOK;
  }

  if(AA->Mark.Count()) {
    if(AA->Mark.Find(ml->msgno)) {
      ml->marks[1] = MMRK_MARK;
      ml->high |= MLST_HIGH_MARK;
    }
  }
}


//  ------------------------------------------------------------------

void GMsgList::ReadMlst(int n) {

  MLst* ml = mlst[n];

  if(ml != NULL)
    return;

  ml = mlst[n] = new MLst;
  throw_new(ml);

  ml->msgno = AA->Msgn.CvtReln(n + 1);

  ml->high = 0;
  update_marks(ml);

  if(AA->Msglistfast()) {
    AA->LoadHdr(&msg, ml->msgno);
  }
  else {
    AA->LoadMsg(&msg, ml->msgno, CFG->dispmargin-(int)CFG->switches.get(disppagebar));
  }
  ml->goldmark = goldmark;

  for(std::vector<Node>::iterator x = CFG->username.begin(); x != CFG->username.end(); x++) {
    if(strieql(msg.By(), x->name)) {
      ml->high |= MLST_HIGH_FROM;
      msg.attr.fmu1();
    }
    if(strieql(msg.to, x->name)) {
      ml->high |= MLST_HIGH_TO;
      msg.attr.tou1();
    }
  }
  if(strieql(msg.to, AA->Internetaddress())) {
    ml->high |= MLST_HIGH_TO;
    msg.attr.tou1();
  }

  // Highlight FROM if local
  if(CFG->switches.get(displocalhigh) and msg.attr.loc())
    ml->high |= MLST_HIGH_FROM;

  // Highlight if unread
  if((msg.timesread == 0) and CFG->switches.get(highlightunread))
    ml->high |= MLST_HIGH_UNREAD;

  // Highlight if unsent
  if(msg.attr.uns() and not msg.attr.rcv() and not msg.attr.del())
    ml->high |= MLST_HIGH_UNSENT;

  ml->written = msg.written;
  ml->arrived = msg.arrived;
  ml->received = msg.received;
  strcpy(ml->by, msg.By());
  strcpy(ml->to, msg.To());
  strcpy(ml->re, msg.re);

  ml->colorby = GetColorName(msg.orig);
  if (ml->colorby == -1) ml->colorby = GetColorName(ml->by);
  ml->colorto = AA->isnet() ? GetColorName(msg.dest) : -1;
  if (ml->colorto == -1) ml->colorto = GetColorName(ml->to);
}


//  ------------------------------------------------------------------

void GMsgList::do_delayed() {

  // Update header and statusline
  if(AA->Msglistheader()) {
    ReadMlst(index);
    AA->LoadMsg(&msg, mlst[index]->msgno, CFG->dispmargin-(int)CFG->switches.get(disppagebar));
    mlst[index]->goldmark = goldmark;
    if(mlst[index]->high & MLST_HIGH_FROM)
      msg.attr.fmu1();
    if(mlst[index]->high & MLST_HIGH_TO)
      msg.attr.tou1();
    int mlstwh = whandle();
    HeaderView->Use(AA, &msg);
    HeaderView->Paint();
    wactiv_(mlstwh);
  }

  if(CFG->switches.get(msglistviewsubj)) {
    ReadMlst(index);
    wtitle(mlst[index]->re, TCENTER|TBOTTOM, tattr);
  }

  if(CFG->switches.get(msglistpagebar))
    wscrollbar(W_VERT, maximum_index+1, maximum_index, index);

  update_statuslinef(LNG->MsgLister, index+1, maximum_index+1, maximum_index-index);
}


//  ------------------------------------------------------------------

void GMsgList::update_title() {

  int bycol = 8;
  int tocol = bycol + mlst_bysiz + 1 + fldadd1;
  int recol = tocol + mlst_tosiz + 1 + fldadd1;
  int dtcol = recol + mlst_resiz + 1 + fldadd2;
  if(AA->Msglistwidesubj())
    recol = tocol;

  window.title(NULL, tattr, TCENTER);
  window.message(CFG->switches.get(disprealmsgno) ? LNG->MsgReal : LNG->Msg, TP_BORD, 3, tattr);
  window.message(LNG->FromL, TP_BORD, bycol, tattr);
  if(not AA->Msglistwidesubj())
    window.message(LNG->ToL, TP_BORD, tocol, tattr);
  window.message(LNG->SubjL, TP_BORD, recol, tattr);
  switch(AA->Msglistdate()) {
    case MSGLISTDATE_WRITTEN:   window.message(LNG->Written, TP_BORD, dtcol, tattr);   break;
    case MSGLISTDATE_ARRIVED:   window.message(LNG->Arrived, TP_BORD, dtcol, tattr);   break;
    case MSGLISTDATE_RECEIVED:  window.message(LNG->Received, TP_BORD, dtcol, tattr);  break;
  }
}


//  ------------------------------------------------------------------

void GMsgList::print_line(uint idx, uint pos, bool isbar) {

  int bycol = 7;
  int tocol = bycol + mlst_bysiz + 1 + fldadd1;
  int bysiz = mlst_bysiz + fldadd1;
  int tosiz = mlst_tosiz + fldadd1;
  int resiz = mlst_resiz + fldadd2;

  ReadMlst(idx);
  MLst* ml = mlst[idx];

  update_marks(ml);

  int wattr_, hattr_, mattr_;
  if(isbar) {
    wattr_ = sattr;
    hattr_ = sattr;
    mattr_ = sattr;
  }
  else if(ml->high & MLST_HIGH_UNSENT) {
    wattr_ = C_MENUW_UNSENT;
    hattr_ = C_MENUQ_UNSENTHIGH;
    mattr_ = hattr;
  }
  else if(ml->high & MLST_HIGH_UNREAD) {
    wattr_ = C_MENUW_UNREAD;
    hattr_ = C_MENUQ_UNREADHIGH;
    mattr_ = hattr;
  }
  else {
    wattr_ = wattr;
    hattr_ = hattr;
    mattr_ = hattr;
  }

  char buf[256];

  if(AA->Msglistwidesubj()) {
    resiz += tosiz + 1;
    tosiz = 0;
  }

  char nbuf[33], dbuf[20];
  strcpy(dbuf, LNG->n_a);

  time32_t dt = 0;
  switch(AA->Msglistdate()) {
    case MSGLISTDATE_WRITTEN:   dt = ml->written;   break;
    case MSGLISTDATE_ARRIVED:   dt = ml->arrived;   break;
    case MSGLISTDATE_RECEIVED:  dt = ml->received;  break;
  }
  if(dt)
    strftimei(dbuf, 20, "%d %b %y", ggmtime(&dt));
  if(AA->Msglistdate())
    strsetsz(dbuf, 10);
  else
    *dbuf = NUL;
  sprintf(nbuf, "%5u", (CFG->switches.get(disprealmsgno) ? ml->msgno : AA->Msgn.ToReln(ml->msgno)));
  sprintf(buf, "%-5.5s%s%-*.*s %-*.*s%s%-*.*s %s",
    nbuf, ml->marks,
    bysiz, bysiz, ml->by,
    tosiz, tosiz, ml->to,
    (tosiz ? " " : ""),
    resiz, resiz, ml->re,
    dbuf
  );

  window.prints(pos, 0, wattr_, buf);

  if (ml->high & (MLST_HIGH_BOOK|MLST_HIGH_MARK))
    window.prints(pos, 5, mattr_, ml->marks);

  if ((ml->high & MLST_HIGH_FROM) || (ml->colorby != -1))
  {
    int color = ((ml->colorby != -1) && !isbar) ? ml->colorby : hattr_;
    window.printns(pos, bycol, color, ml->by, bysiz);
  }

  if (((ml->high & MLST_HIGH_TO) || (ml->colorto != -1)) && 
      !AA->Msglistwidesubj())
  {
    int color = ((ml->colorto != -1) && !isbar) ? ml->colorto : hattr_;
    window.printns(pos, tocol, color, ml->to, tosiz);
  }

  goldmark = ml->goldmark;
}


//  ------------------------------------------------------------------

bool GMsgList::handle_key() {

  gkey kk;

  if(key < KK_Commands) {
    // See if it's a listkey
    kk = SearchKey(key, ListKey, ListKeys);
    if(kk)
      key = kk;
    else {
      // If not a listkey, see if it matches a readkey
      if(not IsMacro(key, KT_M)) {
        kk = SearchKey(key, ReadKey, ReadKeys);
        if(kk)
          key = kk;
      }
    }
  }

  ReadMlst(index);

  switch(key) {
    case KK_ListGotoPrev:
      key = Key_Up;
      default_handle_key();
      break;

    case KK_ListGotoNext:
      key = Key_Dwn;
      default_handle_key();
      break;

    case KK_ListGotoFirst:
      key = Key_Home;
      default_handle_key();
      break;

    case KK_ListGotoLast:
      key = Key_End;
      default_handle_key();
      break;

    case KK_ListAskExit:
      {
        GMenuQuit MenuQuit;
        aborted = gkbd.quitall = make_bool(MenuQuit.Run());
        if(gkbd.quitall) {
          AA->bookmark = AA->Msgn.CvtReln(msgmark2);
          return false;
        }
      }
      break;

    case KK_ListQuitNow:
      gkbd.quitall = true;
      ///////////////// Drop Through

    case KK_ListAbort:
      aborted = true;
      AA->bookmark = AA->Msgn.CvtReln(msgmark2);
      ///////////////// Drop Through

    case KK_ListSelect:
      return false;

    case KK_ListMark:
      {
        uint32_t temp = AA->Mark.Find(mlst[index]->msgno);
        if(not temp) {
          AA->Mark.Add(mlst[index]->msgno);
          update_marks(mlst[index]);
        }
      }
      if(index < maximum_index)
        cursor_down();
      else
        display_bar();
      break;

    case KK_ListUnmark:
      {
        uint32_t temp = AA->Mark.Find(mlst[index]->msgno);
        if(temp) {
          AA->Mark.DelReln(temp);
          update_marks(mlst[index]);
        }
      }
      if(index < maximum_index)
        cursor_down();
      else
        display_bar();
      break;

    case KK_ListToggleMark:
      {
        uint32_t temp = AA->Mark.Find(mlst[index]->msgno);
        if(temp) {
          AA->Mark.DelReln(temp);
        }
        else {
          AA->Mark.Add(mlst[index]->msgno);
        }
        update_marks(mlst[index]);
      }
      if(index < maximum_index)
        cursor_down();
      else
        display_bar();
      break;

    case KK_ListToggleBookMark:
      if(AA->bookmark == mlst[index]->msgno) {
        mlst[index]->marks[0] = ' ';
        AA->bookmark = 0;
        mlst[index]->high &= ~MLST_HIGH_BOOK;
        display_bar();
      }
      else {
        long prevbm = AA->Msgn.ToReln(AA->bookmark-1);
        long newbm = index;
        AA->bookmark = mlst[index]->msgno;
        mlst[index]->marks[0] = MMRK_BOOK;
        mlst[index]->high |= MLST_HIGH_BOOK;
        display_bar();
        if(prevbm) {
          if(in_range((long)position + prevbm - newbm, 0l, (long)maximum_position)) {
            ReadMlst(prevbm);
            mlst[prevbm]->marks[0] = ' ';
            mlst[prevbm]->high &= ~MLST_HIGH_BOOK;
            index = prevbm;
            position += prevbm - newbm;
            display_line();
            index = newbm;
            position -= prevbm - newbm;
          }
        }
      }
      break;

    case KK_ListGotoBookMark:
      if(AA->bookmark) {
        long prevbm = AA->Msgn.ToReln(AA->bookmark-1);
        long newbm = index;
        index = prevbm;
        AA->bookmark = mlst[newbm]->msgno;
        if(in_range((long)position + prevbm - newbm, 0l, (long)maximum_position)) {
          mlst[newbm]->marks[0] = MMRK_BOOK;
          mlst[newbm]->high |= MLST_HIGH_BOOK;
          index = newbm;
          display_line();
          index = prevbm;
          ReadMlst(index);
          mlst[index]->marks[0] = ' ';
          mlst[index]->high &= ~MLST_HIGH_BOOK;
          position += prevbm - newbm;
          display_bar();
        }
        else
          center(CFG->displistcursor);
      }
      else
        SayBibi();
      break;

    case KK_ListMarkingOptions:
      {
        uint lrbak = AA->lastread();
        AA->set_lastread(index + 1);
        msg.msgno = AA->Msgn.CvtReln(AA->lastread());
        MarkMsgs(&msg);
        AA->set_lastread(lrbak);
        update();
      }
      break;

    case KK_ListDosShell:
      DosShell();
      break;

    case KK_ListWideSubj:
      if(not AA->Msglistwidesubj()) {
        AA->ToggleMsglistwidesubj();
        update_title();
        update();
      }
      break;

    case KK_ListNarrowSubj:
      if(AA->Msglistwidesubj()) {
        AA->ToggleMsglistwidesubj();
        update_title();
        update();
      }
      break;

    case KK_ListToggleWideSubj:
      AA->ToggleMsglistwidesubj();
      update_title();
      update();
      break;

    case KK_ListToggleDate:
      AA->NextMsglistdate();
      mlst_with_date(AA->Msglistdate());
      update_title();
      update();
      break;

    case KK_ReadMessageList:
      center(CFG->displistcursor);
      break;

    case Key_Tick:
      CheckTick(KK_ListQuitNow);
      break;

    case KK_ListUndefine:
      break;

    default:
      if(not PlayMacro(key, KT_M)) {
        if(gkbd.kbuf == NULL)
          kbput(key);
        switch(key) {
          case KK_ListAbort:
          case KK_ReadNewArea:
            aborted = true;
        }
        return false;
      }
  }
  return true;
}


//  ------------------------------------------------------------------

void GMsgList::Run() {

  if(maximum_index == 0) {
    aborted = true;
    return;
  }

  if(AA->Msgn.ToReln(reader_msg->msgno) != 0)
    index = AA->Msgn.ToReln(reader_msg->msgno)-1;
  else
    index = 0;
  minimum_index = 0;
  msgmark2 = AA->Msgn.ToReln(AA->bookmark);

  ypos    = AA->Msglistheader() ? 6 : 1;      // Window Starting Row
  xpos    = 0;                                // Window Starting Column
  ylen    = MAXROW-3-ypos;                    // Window Height
  xlen    = MAXCOL-2;                         // Window Width
  btype   = W_BMENU;                          // Window Border Type
  battr   = C_MENUB;                          // Window Border Color
  wattr   = C_MENUW;                          // Window Color
  tattr   = C_MENUT;                          // Window Title Color
  sattr   = C_MENUS;                          // Window Selection Bar Color
  hattr   = C_MENUQ;                          // Window Highlight Color
  sbattr  = C_MENUPB;                         // Window Scrollbar Color
  title   = LNG->ThreadlistTitle;             // Window Title
  helpcat = H_MessageBrowser;                 // Window Help Category
  listwrap  = CFG->switches.get(displistwrap);

  if((AA->Msglistdate() == MSGLISTDATE_RECEIVED) and not AA->havereceivedstamp())
    AA->SetMsglistdate(MSGLISTDATE_WRITTEN);
  else if((AA->Msglistdate() == MSGLISTDATE_ARRIVED) and not AA->havearrivedstamp())
    AA->SetMsglistdate(MSGLISTDATE_WRITTEN);

  mlst_with_date(AA->Msglistdate());

  fldadd1 = (MAXCOL-80)/3;
  fldadd2 = (MAXCOL-80) - (fldadd1*2);

  mlst = (MLst **)throw_malloc(sizeof(MLst *) * (maximum_index + 1));

  for(uint i=0; i<= maximum_index; i++)
    mlst[i] = NULL;

  maximum_position = MinV((uint)maximum_index, (uint)ylen - 1);

  run_picker();

  if(not aborted) {
    ReadMlst(index);
    AA->set_lastread(AA->Msgn.ToReln(mlst[index]->msgno));
  }
}


//  ------------------------------------------------------------------

void MessageBrowse() {

  if(AA->Msgn.Count()) {
    GMsgList p;
    _in_msglist = true;
    p.Run();
    _in_msglist = false;
    if(AA->PMrk.Tags() == 0)
      AA->isreadpm = false;
    if(AA->Mark.Count() == 0)
      AA->isreadmark = false;
    if(gkbd.quitall)
      QuitNow();
  }
}


//  ------------------------------------------------------------------

class ThreadEntry {

public:
  uint32_t msgno;
  uint32_t replyto;
  uint32_t reply1st;
  uint32_t replynext;
  uint32_t replytoindex;
  uint32_t level;
};

#define MAX_LEVEL 20

class GThreadlist : public gwinpick {

private:

  gwindow               window;
  GMsg                  msg;
  std::vector<ThreadEntry>   list;
  ThreadEntry           t;

  void BuildThreadIndex(dword msgno);
  void recursive_build(uint32_t msgn, uint32_t rn, uint32_t level);
  void GenTree(char* buf2, int idx, uint32_t maxlev);
  void update_title();
  bool NextThread(bool next);

public:

  void open();                        // Called after window is opened
  void close();                       // Called after window is closed
  void print_line(uint idx, uint pos, bool isbar);
  void do_delayed();
  bool handle_key();                  // Handles keypress

  void Run();

  GThreadlist() { memset(&msg, 0, sizeof(GMsg)); replylinkfloat = CFG->replylinkfloat; };
  ~GThreadlist() { ResetMsg(&msg); };
};


//  ------------------------------------------------------------------

void GThreadlist::open() {

  window.openxy(ypos, xpos, ylen+2, xlen+2,  btype, battr, 7);
  update_title();

  center(CFG->displistcursor);
}


//  ------------------------------------------------------------------

void GThreadlist::update_title() {

  window.title(title, tattr);
  window.message(CFG->switches.get(disprealmsgno) ? LNG->MsgReal : LNG->Msg, TP_BORD, 3, tattr);

  switch(AA->Msglistdate()) {
    case MSGLISTDATE_WRITTEN:   window.message(LNG->Written, TP_BORD, xlen-9, tattr);   break;
    case MSGLISTDATE_ARRIVED:   window.message(LNG->Arrived, TP_BORD, xlen-9, tattr);   break;
    case MSGLISTDATE_RECEIVED:  window.message(LNG->Received, TP_BORD, xlen-9, tattr);  break;
  }
}


//  ------------------------------------------------------------------

void GThreadlist::do_delayed() {

  // Update header and statusline
  if(AA->Msglistheader()) {
    AA->LoadMsg(&msg, list[index].msgno, CFG->dispmargin-(int)CFG->switches.get(disppagebar));
    for(std::vector<Node>::iterator x = CFG->username.begin(); x != CFG->username.end(); x++) {
      if(strieql(msg.By(), x->name)) {
        msg.attr.fmu1();
      }
      if(strieql(msg.to, x->name)) {
        msg.attr.tou1();
      }
    }
    if(strieql(msg.to, AA->Internetaddress())) {
      msg.attr.tou1();
    }
    int mlstwh = whandle();
    HeaderView->Use(AA, &msg);
    HeaderView->Paint();
    wactiv_(mlstwh);
  }

  if(CFG->switches.get(msglistviewsubj)) {
    // Reload message if not sure that just reread
    if(not AA->Msglistheader()) {
      t = list[index];
      if(AA->Msglistfast()) {
        AA->LoadHdr(&msg, t.msgno);
      }
      else {
        AA->LoadMsg(&msg, t.msgno, CFG->dispmargin-(int)CFG->switches.get(disppagebar));
      }
    }
    wtitle(msg.re, TCENTER|TBOTTOM, tattr);
  }

  if(CFG->switches.get(msglistpagebar))
    wscrollbar(W_VERT, maximum_index+1, maximum_index, index);

  update_statuslinef(LNG->MsgLister, index+1, maximum_index+1, maximum_index-index);
}


//  ------------------------------------------------------------------

void GThreadlist::close() {

  window.close();
  ResetMsg(&msg);
}


//  ------------------------------------------------------------------

void GThreadlist::GenTree(char* buf, int idx, uint32_t maxlev) {

#ifdef KOI8
  static char graph[4]="���";
#else
  static char graph_ibmpc[4]="���";
  static char graph[4]="";

  if(graph[0] == NUL) {
    int table = LoadCharset(NULL, NULL, 1);
    int level = LoadCharset(get_dos_charset(CFG->xlatlocalset), CFG->xlatlocalset);
    XlatStr(graph, graph_ibmpc, level, CharTable);
    if(table == -1)
      LoadCharset(CFG->xlatimport, CFG->xlatlocalset);
    else
      LoadCharset(CFG->xlatcharset[table].imp, CFG->xlatcharset[table].exp);
  }
#endif

  ThreadEntry te = list[idx];

  buf[0] = ' ';

  if(te.level == 0) {
    buf[1] = NUL;
    return;
  }

  if(te.level < maxlev) {
    buf[(te.level-1)*2+1] = (te.replynext) ? graph[0] : graph[1];
    buf[(te.level-1)*2+2] = ' ';
    buf[(te.level-1)*2+3] = NUL;
  }

  while(te.replyto) {
    te = list[te.replytoindex];
    if((te.level != 0) and (te.level < maxlev)) {
      buf[(te.level-1)*2+1] = (te.replynext) ? graph[2] : ' ';
      buf[(te.level-1)*2+2] = ' ';
    }
  }

  buf[maxlev*2+1] = NUL;
}


//  ------------------------------------------------------------------

void GThreadlist::print_line(uint idx, uint pos, bool isbar) {

  char buf[256];
  uint32_t maxlev = (100*window.width()+h_offset+1)/2;
#if defined(__USE_ALLOCA__)
  char *buf2 = (char*)alloca(maxlev*2+2);
#else
  __extension__ char buf2[maxlev*2+2];
#endif

  t = list[idx];
  size_t tdlen = xlen - ((AA->Msglistdate() == MSGLISTDATE_NONE) ? 8 : 18);

  if(AA->Msglistfast()) {
    AA->LoadHdr(&msg, t.msgno);
  }
  else {
    AA->LoadMsg(&msg, t.msgno, CFG->dispmargin-(int)CFG->switches.get(disppagebar));
  }

  int attrh, attrw;
  if(msg.attr.uns() and not msg.attr.rcv() and not msg.attr.del()) {
    attrw = C_MENUW_UNSENT;
    attrh = C_MENUQ_UNSENTHIGH;
  }
  else if(CFG->switches.get(highlightunread) and (msg.timesread == 0)) {
    attrh = C_MENUQ_UNREADHIGH;
    attrw = C_MENUW_UNREAD;
  }
  else {
    attrw = wattr;
    attrh = hattr;
  }

  GenTree(buf2, idx, maxlev);

  #if defined(__UNIX__) && !defined(__USE_NCURSES__)
  gvid_boxcvt(buf2);
  #endif

  char marks[3];

  strcpy(marks, "  ");

  if(AA->bookmark == t.msgno)
    marks[0] = MMRK_BOOK;

  if(AA->Mark.Count()) {
    if(AA->Mark.Find(t.msgno))
      marks[1] = MMRK_MARK;
  }

  sprintf(buf, "%6u  %*c", (CFG->switches.get(disprealmsgno) ? t.msgno : AA->Msgn.ToReln(t.msgno)), tdlen, ' ');

  if(AA->Msglistdate() != MSGLISTDATE_NONE) {
    char dbuf[11];
    time32_t dt = 0;

    memset(dbuf, ' ', 10);
    dbuf[10] = NUL;
    strncpy(dbuf, LNG->n_a, strlen(LNG->n_a));

    switch(AA->Msglistdate()) {
      case MSGLISTDATE_WRITTEN:   dt = msg.written;   break;
      case MSGLISTDATE_ARRIVED:   dt = msg.arrived;   break;
      case MSGLISTDATE_RECEIVED:  dt = msg.received;  break;
    }

    if(dt)
      strftimei(dbuf, 20, "%d %b %y", ggmtime(&dt));
    strcat(buf, dbuf);
  }
  strcat(buf, " ");

  window.prints(pos, 0, isbar ? sattr : attrw, buf);
  window.prints(pos, 6, isbar ? sattr : hattr, marks);

  size_t buf2len = strlen(buf2);
  if (buf2len > h_offset)
  {
    strxcpy(buf, &buf2[h_offset], tdlen);
    window.prints(pos, 8, isbar ? (sattr|ACSET) : (wattr|ACSET), buf);
  }

  int attr = attrw;

  for(std::vector<Node>::iterator x = CFG->username.begin(); x != CFG->username.end(); x++)
    if(strieql(msg.By(), x->name)) {
      attr = attrh;
      break;
    }

  if (!isbar)
  {
    int colorname = GetColorName(msg.orig);
    if (colorname == -1) colorname = GetColorName(msg.By());
    if (colorname != -1) attr = colorname;
  }
  else if (CFG->replylinkfloat)
  {
    size_t bylen = strlen(msg.By());
    if ((buf2len + bylen) > tdlen)
      new_hoffset = (buf2len + bylen)-tdlen+1;
    else
      new_hoffset = 0;

    attr = sattr;
  }

  size_t buflen = strlen(&buf2[h_offset]);
  if (buflen < tdlen)
  {
    strxcpy(buf, msg.By(), tdlen - buflen);
    window.prints(pos, 8 + buflen, attr, buf);
  }
}


//  ------------------------------------------------------------------

void GThreadlist::recursive_build(uint32_t msgn, uint32_t rn, uint32_t level) {

  uint32_t oldmsgno = msg.msgno;

  if(AA->Msgn.ToReln(msgn) and AA->LoadHdr(&msg, msgn)) {

    t.msgno     = msgn;
    t.replyto   = msg.link.to();
    t.reply1st  = msg.link.first();
    t.replynext = rn;
    t.level     = level;
    t.replytoindex = 0;

    if(not AA->Msgn.ToReln(t.replyto))
      t.replyto = 0;
    if(not AA->Msgn.ToReln(t.reply1st))
      t.reply1st = 0;
    if(not AA->Msgn.ToReln(t.replynext))
      t.replynext = 0;

    uint j, list_size = list.size();
    bool found = false;
    for(j=0; j<list_size; j++) {
      if(list[j].msgno == t.replyto) {
        t.replytoindex = j;
        found = true;
        break;
      }
    }

    if(found or (list_size == 0))
      list.push_back(t);

    recursive_build(msg.link.first(), msg.link.list(0), level+1);

    for(int n=0; n < msg.link.list_max()-1; n++) {
      if(msg.link.list(n)) {
        recursive_build(msg.link.list(n), msg.link.list(n+1), level+1);
      } else
        break;
    }
    AA->LoadHdr(&msg, oldmsgno);
  }
}


//  ------------------------------------------------------------------

void GThreadlist::BuildThreadIndex(dword msgn) {

  w_info(LNG->Wait);

  index = maximum_index = position = maximum_position = 0;
  list.clear();

  AA->LoadHdr(&msg, msgn);

  uint32_t msgno = msg.link.to();
  uint32_t prevmsgno = msgn;

  // Search backwards
  while(AA->Msgn.ToReln(msgno)) {

    if(not AA->LoadHdr(&msg, msgno)) {
      msg.link.to_set(0);
      msgno = prevmsgno;
      AA->LoadHdr(&msg, msgno);
      break;
    }

    prevmsgno = msgno;
    msgno = msg.link.to();
  }

  recursive_build(msg.msgno, 0, 0);

  w_info(NULL);

  minimum_index    = 0;
  maximum_index    = list.size() - 1;
  maximum_position = MinV((uint) list.size() - 1, (uint) ylen - 1);
  index            = 0;
  h_offset         = 0;
  new_hoffset      = 0;

  for(uint i = 0; i<list.size(); i++) {
    if(list[i].msgno == msgn)
      index = i;
  }
}


//  ------------------------------------------------------------------

bool GThreadlist::NextThread(bool next) {

  uint m = AA->Msgn.ToReln(reader_msg->msgno);
  for(m = m ? m-1 : 0;
      next ? m < AA->Msgn.Count() : m!=-1;
      next ? m++ : m--) {

    dword msgn = AA->Msgn[m];
    bool found = false;

    for(uint i = 0; i<list.size(); i++) {
      if(list[i].msgno == msgn) {
        found = true;
        break;
      }
    }

    if(not found) {
      reader_msg->msgno = msgn;
      AA->set_lastread(AA->Msgn.ToReln(msgn));

      BuildThreadIndex(msgn);
      return true;
    }
  }
  return true;
}

//  ------------------------------------------------------------------


bool GThreadlist::handle_key() {

  gkey kk;

  if(key < KK_Commands) {
    key = key_tolower(key);
    // See if it's a listkey
    kk = SearchKey(key, ListKey, ListKeys);
    if(kk)
      key = kk;
    else {
      // If not a listkey, see if it matches a readkey
      if(not IsMacro(key, KT_M)) {
        kk = SearchKey(key, ReadKey, ReadKeys);
        if(kk)
          key = kk;
      }
    }
  }

  switch(key) {
    case KK_ListGotoPrev:
    case KK_ListGotoNext:
      NextThread((key == KK_ListGotoNext));
      if (!CFG->replylinkshowalways && (list.size() <= 1))
        return false;
      center(CFG->displistcursor);
      break;

    case KK_ListGotoFirst:
      key = Key_Home;
      default_handle_key();
      break;

    case KK_ListGotoLast:
      key = Key_End;
      default_handle_key();
      break;

    case KK_ListAskExit:
      {
        GMenuQuit MenuQuit;
        aborted = gkbd.quitall = make_bool(MenuQuit.Run());
        if(gkbd.quitall)
          return false;
      }
      break;

    case KK_ListQuitNow:
      gkbd.quitall = true;
      ///////////////// Drop Through

    case KK_ListAbort:
      aborted = true;
      ///////////////// Drop Through

    case KK_ListSelect:
      return false;

    case KK_ListToggleMark:
    {
      uint32_t temp = AA->Mark.Find(list[index].msgno);
      if(temp) {
        AA->Mark.DelReln(temp);
      }
      else {
        AA->Mark.Add(list[index].msgno);
      }

      if(index < maximum_index)
        cursor_down();
      else
        display_bar();
      break;
    }

    case KK_ListDosShell:
      DosShell();
      break;

    case KK_ListToggleDate:
      AA->NextMsglistdate();
      mlst_with_date(AA->Msglistdate());
      update_title();
      update();
      break;

    case Key_Tick:
      CheckTick(KK_ListQuitNow);
      break;

    case KK_ListUndefine:
      break;

    default:
      if(not PlayMacro(key, KT_M)) {
        if(gkbd.kbuf == NULL)
          kbput(key);
        switch(key) {
          case KK_ListAbort:
          case KK_ReadNewArea:
            aborted = true;
        }
        return false;
      }
  }
  return true;
}


//  ------------------------------------------------------------------

void GThreadlist::Run() {

  ypos    = AA->Msglistheader() ? 6 : 1;      // Window Starting Row
  xpos    = 0;                                // Window Starting Column
  ylen    = MAXROW-3-ypos;                    // Window Height
  xlen    = MAXCOL-2;                         // Window Width
  btype   = W_BMENU;                          // Window Border Type
  battr   = C_MENUB;                          // Window Border Color
  wattr   = C_MENUW;                          // Window Color
  tattr   = C_MENUT;                          // Window Title Color
  sattr   = C_MENUS;                          // Window Selection Bar Color
  hattr   = C_MENUQ;                          // Window Highlight Color
  sbattr  = C_MENUPB;                         // Window Scrollbar Color
  title   = LNG->ThreadlistTitle;             // Window Title
  helpcat = H_ReplyThread;                    // Window Help Category
  listwrap  = CFG->switches.get(displistwrap);

  BuildThreadIndex(reader_msg->msgno);

  if(CFG->replylinkshowalways || (list.size() > 1))
    run_picker();
  else {
    w_info(LNG->NoThreadlist);
    waitkeyt(5000);
    w_info(NULL);
    aborted = true;
  }

  if(not aborted)
    AA->set_lastread(AA->Msgn.ToReln(list[index].msgno));
}


//  ------------------------------------------------------------------

void MsgThreadlist() {

  GThreadlist p;

  p.Run();

}


//  ------------------------------------------------------------------
