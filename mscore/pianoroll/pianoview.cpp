//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2009-2013 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================


#include "pianoview.h"
#include "pianoruler.h"
#include "pianokeyboard.h"
#include "shortcut.h"
#include "musescore.h"
#include "scoreview.h"
#include "preferences.h"
#include "libmscore/part.h"
#include "libmscore/staff.h"
#include "libmscore/measure.h"
#include "libmscore/chord.h"
#include "libmscore/rest.h"
#include "libmscore/score.h"
#include "libmscore/note.h"
#include "libmscore/slur.h"
#include "libmscore/tie.h"
#include "libmscore/tuplet.h"
#include "libmscore/segment.h"
#include "libmscore/noteevent.h"
#include "libmscore/utils.h"

namespace Ms {

extern MuseScore* mscore;

static const qreal MIN_DRAG_DIST_SQ = 9;

const BarPattern PianoView::barPatterns[] = {
      {"C maj/A min",   {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1}},
      {"Db maj/Bb min", {1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0}},
      {"D maj/B min",   {0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1}},
      {"Eb maj/C min",  {1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0}},
      {"E maj/Db min",  {0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1}},
      {"F maj/D min",   {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0}},
      {"Gb maj/Eb min", {0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1}},
      {"G maj/E min",   {1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1}},
      {"Ab maj/F min",  {1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0}},
      {"A maj/Gb min",  {0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1}},
      {"Bb maj/G min",  {1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0}},
      {"B maj/Ab min",  {0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1}},
      {"C Diminished",  {1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0}},
      {"Db Diminished", {0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0}},
      {"D Diminished",  {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1}},
      {"C Half/Whole",  {1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0}},
      {"Db Half/Whole", {0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1}},
      {"D Half/Whole",  {1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1}},
      {"C Whole tone",  {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0}},
      {"Db Whole tone", {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}},
      {"C Augmented",   {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}},
      {"Db Augmented",  {0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0}},
      {"D Augmented",   {0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0}},
      {"Eb Augmented",  {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1}},
      {"",              {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}
};

//---------------------------------------------------------
//   PianoItem
//---------------------------------------------------------

PianoItem::PianoItem(Note* n, PianoView* pianoView)
   : _note(n), _pianoView(pianoView)
      {
      }


//---------------------------------------------------------
//   boundingRectTicks
//---------------------------------------------------------

QRect PianoItem::boundingRectTicks(NoteEvent* evt)
      {
      Chord* chord = _note->chord();
      int ticks = chord->ticks().ticks();
      int tieLen = _note->playTicks() - ticks;
      int pitch = _note->pitch() + (evt ? evt->pitch() : 0);
      int len = (evt ? ticks * evt->len() / 1000 : ticks) + tieLen;

      int x1 = _note->chord()->tick().ticks()
            + (evt ? evt->ontime() * ticks / 1000 : 0);
      qreal y1 = pitch;

      QRect rect;
      rect.setRect(x1, y1, len, 1);
      return rect;
      }

//---------------------------------------------------------
//   boundingRectPixels
//---------------------------------------------------------

QRect PianoItem::boundingRectPixels(NoteEvent* evt)
      {
      QRect rect = boundingRectTicks(evt);

      qreal tix2pix = _pianoView->xZoom();
      int noteHeight = _pianoView->noteHeight();

      rect.setRect(_pianoView->tickToPixelX(rect.x()),
              (127 - rect.y()) * noteHeight,
              rect.width() * tix2pix,
              rect.height() * noteHeight
              );

      return rect;
      }

//---------------------------------------------------------
//   boundingRect
//---------------------------------------------------------

QRect PianoItem::boundingRect() {
      Chord* chord = _note->chord();
      int ticks = chord->ticks().ticks();
      int tieLen = _note->playTicks() - ticks;
      int len = ticks + tieLen;
      int pitch = _note->pitch();

      qreal tix2pix = _pianoView->xZoom();
      int noteHeight = _pianoView->noteHeight();

      qreal x1 = _pianoView->tickToPixelX(_note->chord()->tick().ticks());
      qreal y1 = (127 - pitch) * noteHeight;

      QRect rect;
      rect.setRect(x1, y1, len * tix2pix, noteHeight);
      return rect;
      }



//---------------------------------------------------------
//   intersects
//---------------------------------------------------------

bool PianoItem::intersectsBlock(int startTick, int endTick, int highPitch, int lowPitch, NoteEvent* evt)
      {
      QRect r = boundingRectTicks(evt);
      int pitch = r.y();

      return r.right() >= startTick && r.left() <= endTick
            && pitch >= lowPitch && pitch <= highPitch;
      }

//---------------------------------------------------------
//   intersects
//---------------------------------------------------------

bool PianoItem::intersects(int startTick, int endTick, int highPitch, int lowPitch)
      {
      if (_pianoView->playEventsView()) {
            for (NoteEvent& e : _note->playEvents())
                  if (intersectsBlock(startTick, endTick, highPitch, lowPitch, &e))
                        return true;
            return false;
            }
      else
            return intersectsBlock(startTick, endTick, highPitch, lowPitch, 0);

      }


//---------------------------------------------------------
//   getTweakNoteEvent
//---------------------------------------------------------

NoteEvent* PianoItem::getTweakNoteEvent()
      {
      //Get topmost play event for note
      if (_note->playEvents().size() > 0)
            return &(_note->playEvents()[_note->playEvents().size() - 1]);

      return 0;
      }


//---------------------------------------------------------
//   paintNoteBlock
//---------------------------------------------------------

void PianoItem::paintNoteBlock(QPainter* painter, NoteEvent* evt)
      {
      int roundRadius = 3;

      QColor noteDeselected;
      QColor noteSelected;
      QColor tieColor;

      switch (preferences.globalStyle()) {
            case MuseScoreStyleType::DARK_FUSION:
                  noteDeselected = QColor(preferences.getColor(PREF_UI_PIANOROLL_DARK_NOTE_UNSEL_COLOR));
                  noteSelected = QColor(preferences.getColor(PREF_UI_PIANOROLL_DARK_NOTE_SEL_COLOR));
                  tieColor = QColor(preferences.getColor(PREF_UI_PIANOROLL_DARK_BG_TIE_COLOR));
                  break;
            default:
                  noteDeselected = QColor(preferences.getColor(PREF_UI_PIANOROLL_LIGHT_NOTE_UNSEL_COLOR));
                  noteSelected = QColor(preferences.getColor(PREF_UI_PIANOROLL_LIGHT_NOTE_SEL_COLOR));
                  tieColor = QColor(preferences.getColor(PREF_UI_PIANOROLL_LIGHT_BG_TIE_COLOR));
                  break;
            }

      QColor noteColor = _note->selected() ? noteSelected : noteDeselected;
      painter->setBrush(noteColor);

      painter->setPen(QPen(noteColor.darker(250)));
      QRectF bounds = boundingRectPixels(evt);
      painter->drawRoundedRect(bounds, roundRadius, roundRadius);

      //Tie markings
      painter->setPen(QPen(tieColor));

      for (Note* note = _note; note->tieFor(); note = note->tieFor()->endNote()) {
            Chord* chord = note->chord();
            int start = chord->tick().ticks();
            int duration = chord->ticks().ticks();
            int xpos = _pianoView->tickToPixelX(start + duration);

            painter->drawLine(QLineF(xpos, bounds.y(), xpos, bounds.y() + bounds.height()));
            }

      //Pitch name
      if (bounds.width() >= 20 && bounds.height() >= 12) {
            QRectF textRect(bounds.x() + 2, bounds.y(), bounds.width() - 6, bounds.height() + 1);
            QRectF textHiliteRect(bounds.x() + 3, bounds.y() + 1, bounds.width() - 6, bounds.height());

            QFont f("FreeSans", 8);
            painter->setFont(f);

            //Note name
            QString name = tpc2name(_note->tpc(), NoteSpellingType::STANDARD, NoteCaseType::AUTO, false);
            painter->setPen(QPen(noteColor.lighter(130)));
            painter->drawText(textHiliteRect,
                  Qt::AlignLeft | Qt::AlignTop, name);

            painter->setPen(QPen(noteColor.darker(180)));
            painter->drawText(textRect,
                  Qt::AlignLeft | Qt::AlignTop, name);

            //Voice number
            if (bounds.width() >= 26) {
                  painter->setPen(QPen(noteColor.lighter(130)));
                  painter->drawText(textHiliteRect,
                        Qt::AlignRight | Qt::AlignTop, QString::number(_note->voice() + 1));

                  painter->setPen(QPen(noteColor.darker(180)));
                  painter->drawText(textRect,
                        Qt::AlignRight | Qt::AlignTop, QString::number(_note->voice() + 1));
                  }
            }
      }

//---------------------------------------------------------
//   paint
//---------------------------------------------------------

void PianoItem::paint(QPainter* painter)
      {
      painter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);

      if (_pianoView->playEventsView()) {
            for (NoteEvent& e : _note->playEvents())
                  paintNoteBlock(painter, &e);
            }
      else
            paintNoteBlock(painter, 0);
      }


//---------------------------------------------------------
//   PianoView
//---------------------------------------------------------

PianoView::PianoView()
   : QGraphicsView()
      {
      setFrameStyle(QFrame::NoFrame);
      setLineWidth(0);
      setMidLineWidth(0);
      setScene(new QGraphicsScene);
      setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
      setResizeAnchor(QGraphicsView::AnchorUnderMouse);
      setMouseTracking(true);
      _timeType   = TType::TICKS;
      _playEventsView = true;
      _staff      = nullptr;
      _chord      = nullptr;
      _locator    = nullptr;
      _ticks      = 0;
      _barPattern = 0;
      _tuplet     = 1;
      _subdiv     = 0;
      _noteHeight = DEFAULT_KEY_HEIGHT;
      _xZoom      = X_ZOOM_INITIAL;
      _dragStarted = false;
      _lastDragPitch = 0;
      _mouseDown   = false;
      _dragStyle   = DragStyle::NONE;
      _inProgressUndoEvent = false;

      memset(_pitchHighlight, 0, 128);
      }

//---------------------------------------------------------
//   ~PianoView
//---------------------------------------------------------

PianoView::~PianoView()
      {
      clearNoteData();
      }

//---------------------------------------------------------
//   drawBackground
//---------------------------------------------------------

void PianoView::drawBackground(QPainter* p, const QRectF& r)
      {
      if (_staff == 0)
            return;
      Score* _score = _staff->score();
      setFrameShape(QFrame::NoFrame);

      QColor colSelectionBox;

      QColor colWhiteKeyBg;
      QColor colGutter;
      QColor colBlackKeyBg;
      QColor colHilightKeyBg;

      QColor colGridLine;

      switch (preferences.globalStyle()) {
            case MuseScoreStyleType::DARK_FUSION:
                  colSelectionBox = QColor(preferences.getColor(PREF_UI_PIANOROLL_DARK_SELECTION_BOX_COLOR));

                  colHilightKeyBg = QColor(preferences.getColor(PREF_UI_PIANOROLL_DARK_BG_KEY_HIGHLIGHT_COLOR));
                  colWhiteKeyBg = QColor(preferences.getColor(PREF_UI_PIANOROLL_DARK_BG_KEY_WHITE_COLOR));
                  colGutter = QColor(preferences.getColor(PREF_UI_PIANOROLL_DARK_BG_BASE_COLOR));
                  colBlackKeyBg = QColor(preferences.getColor(PREF_UI_PIANOROLL_DARK_BG_KEY_BLACK_COLOR));

                  colGridLine = QColor(preferences.getColor(PREF_UI_PIANOROLL_DARK_BG_GRIDLINE_COLOR));
                  break;
            default:
                  colSelectionBox = QColor(preferences.getColor(PREF_UI_PIANOROLL_LIGHT_SELECTION_BOX_COLOR));

                  colHilightKeyBg = QColor(preferences.getColor(PREF_UI_PIANOROLL_LIGHT_BG_KEY_HIGHLIGHT_COLOR));
                  colWhiteKeyBg = QColor(preferences.getColor(PREF_UI_PIANOROLL_LIGHT_BG_KEY_WHITE_COLOR));
                  colGutter = QColor(preferences.getColor(PREF_UI_PIANOROLL_LIGHT_BG_BASE_COLOR));
                  colBlackKeyBg = QColor(preferences.getColor(PREF_UI_PIANOROLL_LIGHT_BG_KEY_BLACK_COLOR));

                  colGridLine = QColor(preferences.getColor(PREF_UI_PIANOROLL_LIGHT_BG_GRIDLINE_COLOR));
                  break;
            }

      const QColor colSelectionBoxFill = QColor(
                        colSelectionBox.red(), colSelectionBox.green(), colSelectionBox.blue(),
                        128);

      const QPen penLineMajor = QPen(colGridLine, 2.0, Qt::SolidLine);
      const QPen penLineMinor = QPen(colGridLine, 1.0, Qt::SolidLine);
      const QPen penLineSub = QPen(colGridLine, 1.0, Qt::DotLine);

      QRectF r1;
      r1.setCoords(-1000000.0, 0.0, tickToPixelX(0), 1000000.0);
      QRectF r2;
      r2.setCoords(tickToPixelX(_ticks), 0.0, 1000000.0, 1000000.0);

      p->fillRect(r, colWhiteKeyBg);
      if (r.intersects(r1))
            p->fillRect(r.intersected(r1), colGutter);
      if (r.intersects(r2))
            p->fillRect(r.intersected(r2), colGutter);

      //
      // draw horizontal grid lines
      //
      qreal y1 = r.y();
      qreal y2 = y1 + r.height();
      qreal x1 = qMax(r.x(), (qreal)tickToPixelX(0));
      qreal x2 = qMin(x1 + r.width(), (qreal)tickToPixelX(_ticks));

      int topPitch = ceil((_noteHeight * 128 - y1) / _noteHeight);
      int bmPitch = floor((_noteHeight * 128 - y2) / _noteHeight);

      Part* part = _staff->part();
      Interval transp = part->instrument()->transpose();

      //MIDI notes span [0, 127] and map to pitches starting at C-1
      for (int pitch = bmPitch; pitch <= topPitch; ++pitch) {
            int y = (127 - pitch) * _noteHeight;

            int degree = (pitch - transp.chromatic + 60) % 12;
            const BarPattern& pat = barPatterns[_barPattern];

            if (!pat.isWhiteKey[degree] || _pitchHighlight[pitch]) {
                  qreal px0 = qMax(r.x(), (qreal)tickToPixelX(0));
                  qreal px1 = qMin(r.x() + r.width(), (qreal)tickToPixelX(_ticks));
                  QRectF hbar;

                  hbar.setCoords(px0, y, px1, y + _noteHeight);
                  p->fillRect(hbar,
                        _pitchHighlight[pitch] ? colHilightKeyBg : colBlackKeyBg);
            }

            //Lines between rows
            p->setPen(degree == 0 ? penLineMajor : penLineMinor);
            p->drawLine(QLineF(x1, y + _noteHeight, x2, y + _noteHeight));
            }

      //
      // draw vertical grid lines
      //
      Pos pos1(_score->tempomap(), _score->sigmap(), qMax(pixelXToTick(x1), 0), TType::TICKS);
      Pos pos2(_score->tempomap(), _score->sigmap(), qMax(pixelXToTick(x2), 0), TType::TICKS);

      int bar1, bar2, beat, tick;
      pos1.mbt(&bar1, &beat, &tick);
      pos2.mbt(&bar2, &beat, &tick);

      //Draw bar lines
      const int minBeatGap = 20;

      for (int bar = bar1; bar <= bar2; ++bar) {
            Pos barPos(_score->tempomap(), _score->sigmap(), bar, 0, 0);

            //Beat lines
            int beatsInBar = barPos.timesig().timesig().numerator();
            int ticksPerBeat = barPos.timesig().timesig().beatTicks();
            double pixPerBeat = ticksPerBeat * _xZoom;
            int beatSkip = ceil(minBeatGap / pixPerBeat);


//            int subExp = qMin((int)floor(log2(pixPerBeat / minBeatGap)), _subBeats);
//            int numSubBeats = pow(2, subExp);

            //Round up to next power of 2
            beatSkip = (int)pow(2, ceil(log(beatSkip)/log(2)));

            for (int beat1 = 0; beat1 < beatsInBar; beat1 += beatSkip) {
                  Pos beatPos(_score->tempomap(), _score->sigmap(), bar, beat1, 0);
                  double x = tickToPixelX(beatPos.time(TType::TICKS));
                  p->setPen(penLineMinor);
                  p->drawLine(x, y1, x, y2);

                  int subbeats = _tuplet * (1 << _subdiv);

                  for (int sub = 1; sub < subbeats; ++sub) {
                      Pos subBeatPos(_score->tempomap(), _score->sigmap(), bar, beat1, sub * MScore::division / subbeats);
                      x = tickToPixelX(subBeatPos.time(TType::TICKS));

                      p->setPen(penLineSub);
                      p->drawLine(x, y1, x, y2);
                      }

                  }

            //Bar line
            double x = tickToPixelX(barPos.time(TType::TICKS));
            p->setPen(x > 0 ? penLineMajor : QPen(Qt::black, 2.0));
            p->drawLine(x, y1, x, y2);
            }

      //Draw notes
      for (int i = 0; i < _noteList.size(); ++i)
            _noteList[i]->paint(p);

      //Draw locators
      for (int i = 0; i < 3; ++i) {
            if (_locator[i].valid())
                  {
                  p->setPen(QPen(i == 0 ? Qt::red : Qt::blue, 2));
                  qreal x = tickToPixelX(_locator[i].time(TType::TICKS));
                  p->drawLine(x, y1, x, y2);
                  }
            }

      //Draw drag selection box
      if (_dragStarted && _dragStyle == DragStyle::SELECTION_RECT && _editNoteTool == PianoRollEditTool::SELECT) {
            int minX = qMin(_mouseDownPos.x(), _lastMousePos.x());
            int minY = qMin(_mouseDownPos.y(), _lastMousePos.y());
            int maxX = qMax(_mouseDownPos.x(), _lastMousePos.x());
            int maxY = qMax(_mouseDownPos.y(), _lastMousePos.y());
            QRectF rect(minX, minY, maxX - minX + 1, maxY - minY + 1);

            p->setPen(QPen(colSelectionBox, 2));
            p->setBrush(QBrush(colSelectionBoxFill, Qt::SolidPattern));
            p->drawRect(rect);
            }
      }


//---------------------------------------------------------
//   moveLocator
//---------------------------------------------------------

void PianoView::moveLocator(int /*i*/)
      {
      scene()->update();
      }


//---------------------------------------------------------
//   pixelXToTick
//---------------------------------------------------------

int PianoView::pixelXToTick(int pixX)
      {
      return static_cast<int>(pixX / _xZoom) - MAP_OFFSET;
      }


//---------------------------------------------------------
//   tickToPixelX
//---------------------------------------------------------

int PianoView::tickToPixelX(int tick)
      {
      return static_cast<int>(tick + MAP_OFFSET) * _xZoom;
      }


//---------------------------------------------------------
//   zoomView
//---------------------------------------------------------

void PianoView::zoomView(int step, bool horizontal, int centerX, int centerY)
    {
    if (horizontal) {
            //Horizontal zoom
            QRectF viewRect = mapToScene(viewport()->geometry()).boundingRect();

            int mouseXTick = pixelXToTick(centerX + (int)viewRect.x());

            _xZoom *= pow(X_ZOOM_RATIO, step);
            emit xZoomChanged(_xZoom);

            updateBoundingSize();
            updateNotes();

            int mousePixX = tickToPixelX(mouseXTick);
            horizontalScrollBar()->setValue(mousePixX - centerX);

            scene()->update();
            }
      else {
            //Vertical zoom
            QRectF viewRect = mapToScene(viewport()->geometry()).boundingRect();
            qreal mouseYNote = (centerY + (int)viewRect.y()) / (qreal)_noteHeight;

            _noteHeight = qMax(qMin(_noteHeight + step, MAX_KEY_HEIGHT), MIN_KEY_HEIGHT);
            emit noteHeightChanged(_noteHeight);

            updateBoundingSize();
            updateNotes();

            int mousePixY = static_cast<int>(mouseYNote * _noteHeight);
            verticalScrollBar()->setValue(mousePixY - centerY);

            scene()->update();
            }

      }

//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------

void PianoView::wheelEvent(QWheelEvent* event)
      {
      int step = event->delta() / 120;

      if (event->modifiers() == 0) {
            //Vertical scroll
            QGraphicsView::wheelEvent(event);
            }
      else if (event->modifiers() == Qt::ShiftModifier) {
            //Horizontal scroll
            QWheelEvent we(event->pos(), event->delta(), event->buttons(), 0, Qt::Horizontal);
            QGraphicsView::wheelEvent(&we);
            }
      else if (event->modifiers() == Qt::ControlModifier) {
            //Vertical zoom
            zoomView(step, false, event->x(), event->y());
            }
      else if (event->modifiers() == (Qt::ShiftModifier | Qt::ControlModifier)) {
            //Horizontal zoom
            zoomView(step, true, event->x(), event->y());
            }
      }

//---------------------------------------------------------
//   showPopupMenu
//---------------------------------------------------------

void PianoView::showPopupMenu(const QPoint& pos)
      {
      QMenu popup(this);

      QAction*  act;

      act = new QAction(tr("Note Tweaker"));
      connect(act, &QAction::triggered, this, &PianoView::showNoteTweaker);
      popup.addAction(act);

      popup.addAction(getAction("paste"));
      popup.addAction(getAction("delete"));

      popup.exec(pos);
      }

//---------------------------------------------------------
//   contextMenuEvent
//---------------------------------------------------------

void PianoView::contextMenuEvent(QContextMenuEvent *event)
      {
      showPopupMenu(event->globalPos());
      }


//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void PianoView::mousePressEvent(QMouseEvent* event)
      {
      bool rightBn = event->button() == Qt::RightButton;
      if (!rightBn) {
            _mouseDown = true;
            _mouseDownPos = mapToScene(event->pos());
            _lastMousePos = _mouseDownPos;
            }
      }

//---------------------------------------------------------
//   mouseReleaseEvent
//---------------------------------------------------------

void PianoView::mouseReleaseEvent(QMouseEvent* event)
      {
      int modifiers = QGuiApplication::keyboardModifiers();
      bool bnShift = modifiers & Qt::ShiftModifier;
      bool bnCtrl = modifiers & Qt::ControlModifier;

      bool rightBn = event->button() == Qt::RightButton;
      if (rightBn) {
            //Right clicks have been handled as popup menu
            return;
            }


      NoteSelectType selType = bnShift ? (bnCtrl ? NoteSelectType::SUBTRACT : NoteSelectType::XOR)
              : (bnCtrl ? NoteSelectType::ADD : NoteSelectType::REPLACE);

      if (_dragStarted) {
            if (_dragStyle == DragStyle::SELECTION_RECT) {
                  //Update selection
                  qreal minX = qMin(_mouseDownPos.x(), _lastMousePos.x());
                  qreal minY = qMin(_mouseDownPos.y(), _lastMousePos.y());
                  qreal maxX = qMax(_mouseDownPos.x(), _lastMousePos.x());
                  qreal maxY = qMax(_mouseDownPos.y(), _lastMousePos.y());

                  int startTick = pixelXToTick((int)minX);
                  int endTick = pixelXToTick((int)maxX);
                  int lowPitch = (int)floor(128 - maxY / noteHeight());
                  int highPitch = (int)ceil(128 - minY / noteHeight());

                  selectNotes(startTick, endTick, lowPitch, highPitch, selType);
                  }
            else if (_dragStyle == DragStyle::NOTES) {
                  //Keep last note drag event, if any
                  if (_inProgressUndoEvent)
                        _inProgressUndoEvent = false;
                  }

            _dragStarted = false;
            }
      else {
            //This was just a click, not a drag
            switch (_editNoteTool) {
            case SELECT:
                  handleSelectionClick();
                  break;
            case CHANGE_LENGTH:
                  changeChordLength(_mouseDownPos);
                  break;
            case ERASE:
                  eraseNote(_mouseDownPos);
                  break;
            case INSERT_NOTE:
                  insertNote(modifiers);
                  break;
            case APPEND_NOTE:
                  appendNoteToChord(_mouseDownPos);
                  break;
            case CUT_CHORD:
                  cutChord(_mouseDownPos);
                  break;
            case TIE:
                  toggleTie(_mouseDownPos);
                  break;
            default:
                break;
                }
            }


      _dragStyle = DragStyle::NONE;
      _mouseDown = false;
      scene()->update();
      }


//---------------------------------------------------------
//   mouseMoveEvent
//---------------------------------------------------------

void PianoView::mouseMoveEvent(QMouseEvent* event)
      {
      _lastMousePos = mapToScene(event->pos());

      if (_mouseDown && !_dragStarted) {
            qreal dx = _lastMousePos.x() - _mouseDownPos.x();
            qreal dy = _lastMousePos.y() - _mouseDownPos.y();

            if (dx * dx + dy * dy >= MIN_DRAG_DIST_SQ) {
                  //Start dragging
                  _dragStarted = true;

                  //Check for move note
                  int tick = pixelXToTick(_mouseDownPos.x());
                  int mouseDownPitch = pixelYToPitch(_mouseDownPos.y());
                  PianoItem* pi = pickNote(tick, mouseDownPitch);
                  if (pi) {
                        if (!pi->note()->selected()) {
                              selectNotes(tick, tick, mouseDownPitch, mouseDownPitch, NoteSelectType::REPLACE);
                              }
                        _dragStyle = DragStyle::NOTES;
                        _lastDragPitch = mouseDownPitch;
                        }
                  else {
                        _dragStyle = DragStyle::SELECTION_RECT;
                        }
                  }
            }

      if (_dragStarted) {
          switch (_editNoteTool) {
          case SELECT:
                if (_dragStyle == DragStyle::NOTES)
                      dragSelectionNoteGroup();
                scene()->update();
                break;
          case CHANGE_LENGTH:
                changeChordLength(_lastMousePos);
                break;
          case ERASE:
                eraseNote(_lastMousePos);
                break;
          default:
                break;
                }
            }


      //Update mouse tracker
      QPointF p(mapToScene(event->pos()));
      int pitch = static_cast<int>((_noteHeight * 128 - p.y()) / _noteHeight);
      emit pitchChanged(pitch);

      int tick = pixelXToTick(p.x());
      if (tick < 0) {
            tick = 0;
            trackingPos.setTick(tick);
            trackingPos.setInvalid();
            }
      else
            trackingPos.setTick(tick);
      emit trackingPosChanged(trackingPos);
      }


//---------------------------------------------------------
//   dragSelectionNoteGroup
//---------------------------------------------------------

void PianoView::dragSelectionNoteGroup() {
      int curPitch = pixelYToPitch(_lastMousePos.y());
      if (curPitch != _lastDragPitch) {
            int pitchDelta = curPitch - _lastDragPitch;

            Score* score = _staff->score();
            if (_inProgressUndoEvent) {
                  _inProgressUndoEvent = false;
                  }

            score->startCmd();
            score->upDownDelta(pitchDelta);
            score->endCmd();

            _inProgressUndoEvent = true;
            _lastDragPitch = curPitch;
            }
      scene()->update();

      }


//---------------------------------------------------------
//   eraseNote
//---------------------------------------------------------

void PianoView::eraseNote(const QPointF& pos) {
      Score* score = _staff->score();
      int pickTick = pixelXToTick((int)pos.x());
      int pickPitch = pixelYToPitch(pos.y());
      PianoItem *pn = pickNote(pickTick, pickPitch);

      if (pn) {
            score->startCmd();
            score->deleteItem(pn->note());
            score->endCmd();
            }
      }

//---------------------------------------------------------
//   changeChordLength
//---------------------------------------------------------

void PianoView::changeChordLength(const QPointF& pos) {
      Score* score = _staff->score();
      int pickTick = pixelXToTick((int)pos.x());
      int pickPitch = pixelYToPitch(pos.y());
      PianoItem *pn = pickNote(pickTick, pickPitch);

      if (pn) {
            Note* note = pn->note();
            int track = _staff->idx() * VOICES + note->voice();
            Fraction frac = noteEditLength();
            Chord* chord = note->chord();
            if (chord->ticks() != frac) {
                  //Copy existing cord
                  QList<NoteVal> nvList;
                  for (Note* n: chord->notes())
                      nvList.push_back(n->noteVal());

                  //Rebuild chord
                  score->startCmd();
                  score->deleteItem(chord);
                  Fraction startTick = chord->segment()->tick();

                  for (int i = 0; i < nvList.length(); ++i) {
                        if (i == 0) {
                              ChordRest* cr = score->findCR(startTick, track);
                              score->setNoteRest(cr->segment(), track, nvList.at(i), frac);
                              chord = toChord(score->findCR(startTick, track));
                              }
                        else
                              score->addNote(chord, nvList.at(i));
                        }
                  score->endCmd();
                  }
            }
      }


//---------------------------------------------------------
//   roundToStartBeat
//---------------------------------------------------------

Fraction PianoView::roundToStartBeat(int tick)  const
      {
      Score* _score = _staff->score();
      Pos barPos(_score->tempomap(), _score->sigmap(), tick, TType::TICKS);

      int beatsInBar = barPos.timesig().timesig().numerator();

      //Number of smaller pieces the beat is divided into
      int subbeats = _tuplet * (1 << _subdiv);
      int divisions = beatsInBar * subbeats;

      //Round down to nearest division
      Fraction pickFrac = Fraction::fromTicks(tick);
      int numDiv = (int)floor((pickFrac.numerator() * divisions / (double)pickFrac.denominator()));
      return Fraction(numDiv, divisions);
      }


//---------------------------------------------------------
//   noteEditLength
//---------------------------------------------------------

Fraction PianoView::noteEditLength() const
      {
      //Find n, d such that n/d = 2^_editNoteLength
      int n = _editNoteLength > 0 ? (1 << _editNoteLength) : 1;
      int d = _editNoteLength < 0 ? (1 << -_editNoteLength) : 1;

      //dots multiplier is (2^(n + 1) - 1)/(2^n) where n is the number of dots
      int dotN = (1 << (_editNoteDots + 1)) - 1;
      int dotD = 1 << _editNoteDots;

      return Fraction(n * dotN, d * dotD);
      }


//---------------------------------------------------------
//   appendNoteToChord
//---------------------------------------------------------

void PianoView::appendNoteToChord(const QPointF& pos) {
      Score* score = _staff->score();

      int pickTick = pixelXToTick((int)pos.x());
      int pickPitch = pixelYToPitch(_mouseDownPos.y());
      int voice = _editNoteVoice;

      //Find best chord to add to
      int track = _staff->idx() * VOICES + voice;

      Fraction pt = Fraction::fromTicks(pickTick);
      Segment* seg = score->tick2segment(pt);
      score->expandVoice(seg, track);

      ChordRest* e = score->findCR(pt, track);

      if (e && e->isChord()) {
            Chord* ch = toChord(e);

            if (pt >= e->tick() && pt < (ch->tick() + ch->ticks())) {
                  NoteVal nv(pickPitch);
                  score->startCmd();
                  score->addNote(ch, nv);
                  score->endCmd();
                  }
            }
      else if (e && e->isRest()) {
            Rest* r = toRest(e);
            NoteVal nv(pickPitch);
            score->startCmd();
            score->setNoteRest(r->segment(), track, nv, r->ticks());
            score->endCmd();
            }
      }

//---------------------------------------------------------
//   insertNote
//---------------------------------------------------------

void PianoView::insertNote(int modifiers)
      {
      bool bnShift = modifiers & Qt::ShiftModifier;

      Score* score = _staff->score();

      int pickTick = pixelXToTick((int)_mouseDownPos.x());
      int pickPitch = pixelYToPitch(_mouseDownPos.y());


      if (bnShift) {
            //If shift is held, select note instead
            PianoItem *pn = pickNote(pickTick, pickPitch);
            if (pn) {
                  mscore->play(pn->note());
                  score->setPlayNote(false);

                  selectNotes(pickTick, pickTick + 1, pickPitch, pickPitch, NoteSelectType::REPLACE);
                  }
            return;
            }


      Fraction insertPosition = roundToStartBeat(pickTick);

      int voice = _editNoteVoice;
      int track = _staff->idx() * VOICES + voice;

      NoteVal nv(pickPitch);

      Segment* seg = score->tick2segment(insertPosition);
      score->expandVoice(seg, track);

      ChordRest* e = score->findCR(insertPosition, track);
      if (e) {

            score->startCmd();

            ChordRest* cr0;
            ChordRest* cr1;

            Fraction noteLen = noteEditLength();

            //Default to quarter note if faction is invalid
            if (!noteLen.isValid() || noteLen.isZero())
                  noteLen.set(1, 4);

            if (cutChordRest(e, track, insertPosition, cr0, cr1)) {
                  score->setNoteRest(cr1->segment(), track, nv, noteLen);
                  }
            else {
                  if (cr0->isChord() && cr0->ticks() == noteLen) {
                        Chord* ch = toChord(cr0);
                        score->addNote(ch, nv);
                        }
                  else {
                        score->setNoteRest(cr0->segment(), track, nv, noteLen);
                        }
                  }

            score->endCmd();
            }
      }


//---------------------------------------------------------
//   toggleTie
//---------------------------------------------------------

void PianoView::toggleTie(const QPointF& pos) {
      Score* score = _staff->score();

      int pickTick = pixelXToTick((int)pos.x());
      int pickPitch = pixelYToPitch(pos.y());
      PianoItem *pn = pickNote(pickTick, pickPitch);

      if (pn) {
            score->startCmd();
            toggleTie(pn->note());
            score->endCmd();
            }
      }


//---------------------------------------------------------
//   toggleTie
//---------------------------------------------------------

void PianoView::toggleTie(Note* note) {
      //Based on Score::cmdToggleTie()

      Score* score = _staff->score();

      Tie* tie = note->tieFor();
      if (tie)
            score->undoRemoveElement(tie);
      else {
            Note* note2 = searchTieNote(note);

            if (note2) {
                  tie = new Tie(score);
                  tie->setStartNote(note);
                  tie->setEndNote(note2);
                  tie->setTrack(note->track());
                  tie->setTick(note->chord()->segment()->tick());
                  tie->setTicks(note2->chord()->segment()->tick() - note->chord()->segment()->tick());
                  score->undoAddElement(tie);
                  }
            }
      }

//---------------------------------------------------------
//   cutChord
//---------------------------------------------------------

void PianoView::cutChord(const QPointF& pos) {
      Score* score = _staff->score();

      int pickTick = pixelXToTick((int)pos.x());
      int pickPitch = pixelYToPitch(pos.y());
      PianoItem *pn = pickNote(pickTick, pickPitch);

      int voice = pn ? pn->note()->voice() : _editNoteVoice;

      //Find best chord to add to
      int track = _staff->idx() * VOICES + voice;

      Fraction insertPosition = roundToStartBeat(pickTick);

      Segment* seg = score->tick2segment(insertPosition);
      score->expandVoice(seg, track);

      ChordRest* e = score->findCR(insertPosition, track);
      if (e && !e->tuplet() && _tuplet == 1) {
            score->startCmd();
            Fraction startTick = e->tick();

            if (insertPosition != startTick) {
                  ChordRest* cr0;
                  ChordRest* cr1;
                  cutChordRest(e, track, insertPosition, cr0, cr1);
                  }
            score->endCmd();
            }
      }


//---------------------------------------------------------
//   handleSelectionClick
//---------------------------------------------------------

void PianoView::handleSelectionClick()
{
      int modifiers = QGuiApplication::keyboardModifiers();
      bool bnShift = modifiers & Qt::ShiftModifier;
      bool bnCtrl = modifiers & Qt::ControlModifier;
      NoteSelectType selType = bnShift ? (bnCtrl ? NoteSelectType::SUBTRACT : NoteSelectType::XOR)
              : (bnCtrl ? NoteSelectType::ADD : NoteSelectType::REPLACE);

      Score* score = _staff->score();

      int pickTick = pixelXToTick((int)_mouseDownPos.x());
      int pickPitch = pixelYToPitch(_mouseDownPos.y());

      PianoItem *pn = pickNote(pickTick, pickPitch);

      if (pn) {
            if (selType == NoteSelectType::REPLACE)
                  selType = NoteSelectType::FIRST;

            mscore->play(pn->note());
            score->setPlayNote(false);

            selectNotes(pickTick, pickTick + 1, pickPitch, pickPitch, selType);
            }
      else {
            if (!bnShift && !bnCtrl) {
                  //Select an empty pixel - should clear selection
                  selectNotes(pickTick, pickTick + 1, pickPitch, pickPitch, selType);
                  }
            else if (!bnShift && bnCtrl) {

                  //Insert a new note at nearest subbeat
                  Fraction insertPosition = roundToStartBeat(pickTick);

                  InputState& is = score->inputState();
                  int voice = _editNoteVoice;
                  int track = _staff->idx() * VOICES + voice;

                  NoteVal nv(pickPitch);

                  Segment* seg = score->tick2segment(insertPosition);
                  score->expandVoice(seg, track);

                  ChordRest* e = score->findCR(insertPosition, track);
                  if (e && !e->tuplet() && _tuplet == 1) {
                        //Ignore tuplets
                        score->startCmd();

                        ChordRest* cr0;
                        ChordRest* cr1;
                        Fraction frac = is.duration().fraction();

                        //Default to quarter note if faction is invalid
                        if (!frac.isValid() || frac.isZero())
                              frac.set(1, 4);

                        if (cutChordRest(e, track, insertPosition, cr0, cr1)) {
                              score->setNoteRest(cr1->segment(), track, nv, frac);
                              }
                        else {
                              if (cr0->isChord() && cr0->ticks().ticks() == frac.ticks()) {
                                    Chord* ch = toChord(cr0);
                                    score->addNote(ch, nv);
                                    }
                              else {
                                    score->setNoteRest(cr0->segment(), track, nv, frac);
                                    }
                              }

                        score->endCmd();
                        }

                  }
            else if (bnShift && !bnCtrl) {
                  //Append a pitch to our current chord/rest
                  int voice = _editNoteVoice;

                  //Find best chord to add to
                  int track = _staff->idx() * VOICES + voice;

                  Fraction pt = Fraction::fromTicks(pickTick);
                  Segment* seg = score->tick2segment(pt);
                  score->expandVoice(seg, track);

                  ChordRest* e = score->findCR(pt, track);

                  if (e && e->isChord()) {
                        Chord* ch = toChord(e);

                        if (pt >= e->tick() && pt < (ch->tick() + ch->ticks())) {
                              NoteVal nv(pickPitch);
                              score->startCmd();
                              score->addNote(ch, nv);
                              score->endCmd();
                              }

                        }
                  else if (e && e->isRest()) {
                        Rest* r = toRest(e);
                        NoteVal nv(pickPitch);
                        score->startCmd();
                        score->setNoteRest(r->segment(), track, nv, r->ticks());
                        score->endCmd();
                        }
                  }
            else if (bnShift && bnCtrl) {
                  //Cut the chord/rest at the nearest subbeat
                  int voice = _editNoteVoice;

                  //Find best chord to add to
                  int track = _staff->idx() * VOICES + voice;

                  Fraction insertPosition = roundToStartBeat(pickTick);

                  Segment* seg = score->tick2segment(insertPosition);
                  score->expandVoice(seg, track);

                  ChordRest* e = score->findCR(insertPosition, track);
                  if (e && !e->tuplet() && _tuplet == 1) {
                        score->startCmd();
                        Fraction startTick = e->tick();

                        if (insertPosition != startTick) {
                              ChordRest* cr0;
                              ChordRest* cr1;
                              cutChordRest(e, track, insertPosition, cr0, cr1);
                              }
                        score->endCmd();
                        }
                  }
            }
      }


//---------------------------------------------------------
//   cutChordRest
//---------------------------------------------------------

bool PianoView::cutChordRest(ChordRest* e, int track, Fraction cutTick, ChordRest*& cr0, ChordRest*& cr1)
      {
      Fraction startTick = e->segment()->tick();
      Fraction tcks = e->ticks();
      if (cutTick <= startTick || cutTick > startTick + tcks) {
            cr0 = e;
            cr1 = 0;
            return false;
            }

      //Deselect note being cut
      if (e->isChord()) {
            Chord* ch = toChord(e);
            for (Note* n: ch->notes()) {
                  n->setSelected(false);
                  }
            }
      else if (e->isRest()) {
            Rest* r = toRest(e);
            r->setSelected(false);
            }


      //Subdivide at the cut tick
      NoteVal nv(-1);

      Score* score = _staff->score();
      score->setNoteRest(e->segment(), track, nv, cutTick - e->tick());
      ChordRest *nextCR = score->findCR(cutTick, track);

      Chord* ch0 = 0;

      if (nextCR->isChord()) {
            //Copy chord into initial segment
            Chord* ch1 = toChord(nextCR);

            for (Note* n: ch1->notes()) {
                  NoteVal nx = n->noteVal();
                  if (!ch0) {
                        ChordRest* cr = score->findCR(startTick, track);
                        score->setNoteRest(cr->segment(), track, nx, cr->ticks());
                        ch0 = toChord(score->findCR(startTick, track));
                        }
                  else {
                        score->addNote(ch0, nx);
                        }
                  }
            }

      cr0 = ch0;
      cr1 = nextCR;
      return true;
      }

//---------------------------------------------------------
//   selectNotes
//---------------------------------------------------------

PianoItem* PianoView::pickNote(int tick, int pitch)
      {
      for (int i = 0; i < _noteList.size(); ++i) {
            PianoItem* pi = _noteList[i];

            if (pi->intersects(tick, tick, pitch, pitch))
                  return pi;
            }

      return 0;
      }

//---------------------------------------------------------
//   selectNotes
//---------------------------------------------------------

void PianoView::selectNotes(int startTick, int endTick, int lowPitch, int highPitch, NoteSelectType selType)
      {
      Score* score = _staff->score();
      //score->masterScore()->cmdState().reset();      // DEBUG: should not be necessary
      score->startCmd();

      QList<PianoItem*> oldSel;
      for (int i = 0; i < _noteList.size(); ++i) {
            PianoItem* pi = _noteList[i];
            if (pi->note()->selected())
                  oldSel.append(pi);
            }

      Selection& selection = score->selection();
      selection.deselectAll();

      for (int i = 0; i < _noteList.size(); ++i) {
            PianoItem* pi = _noteList[i];
            bool inBounds = pi->intersects(startTick, endTick, highPitch, lowPitch);

            bool sel;
            switch (selType) {
                  default:
                  case NoteSelectType::REPLACE:
                        sel = inBounds;
                        break;
                  case NoteSelectType::XOR:
                        sel = inBounds != oldSel.contains(pi);
                        break;
                  case NoteSelectType::ADD:
                        sel = inBounds || oldSel.contains(pi);
                        break;
                  case NoteSelectType::SUBTRACT:
                        sel = !inBounds && oldSel.contains(pi);
                        break;
                  case NoteSelectType::FIRST:
                        sel = inBounds && selection.elements().empty();
                        break;
                  }

            if (sel)
                  selection.add(pi->note());
            }

      for (MuseScoreView* view : score->getViewer())
            view->updateAll();

      scene()->update();
      score->setUpdateAll();
      score->update();
      score->endCmd();

      emit selectionChanged();
      }

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void PianoView::leaveEvent(QEvent* event)
      {
      emit pitchChanged(-1);
      trackingPos.setInvalid();
      emit trackingPosChanged(trackingPos);
      QGraphicsView::leaveEvent(event);
      }

//---------------------------------------------------------
//   ensureVisible
//---------------------------------------------------------

void PianoView::ensureVisible(int tick)
      {
      QRectF rect = mapToScene(viewport()->geometry()).boundingRect();

      qreal xpos = tickToPixelX(tick);
      qreal margin = rect.width() / 2;
      if (xpos < rect.x() + margin)
            horizontalScrollBar()->setValue(qMax(xpos - margin, 0.0));
      else if (xpos >= rect.x() + rect.width() - margin)
            horizontalScrollBar()->setValue(qMax(xpos - rect.width() + margin, 0.0));
      }

//---------------------------------------------------------
//   updateBoundingSize
//---------------------------------------------------------
void PianoView::updateBoundingSize()
      {
      Measure* lm = _staff->score()->lastMeasure();
      _ticks       = (lm->tick() + lm->ticks()).ticks();
      scene()->setSceneRect(0.0, 0.0,
              double((_ticks + MAP_OFFSET * 2) * _xZoom),
              _noteHeight * 128);
      }

//---------------------------------------------------------
//   setStaff
//---------------------------------------------------------

void PianoView::setStaff(Staff* s, Pos* l)
      {
      _locator = l;

      if (_staff == s)
            return;

      _staff    = s;
      setEnabled(_staff != nullptr);
      if (!_staff) {
            scene()->blockSignals(true);  // block changeSelection()
            scene()->clear();
            clearNoteData();
            scene()->blockSignals(false);
            return;
            }

      trackingPos.setContext(_staff->score()->tempomap(), _staff->score()->sigmap());
      updateBoundingSize();

      updateNotes();

      QRectF boundingRect;
      bool brInit = false;
      QRectF boundingRectSel;
      bool brsInit = false;

      foreach (PianoItem* item, _noteList) {
            if (!brInit) {
                  boundingRect = item->boundingRect();
                  brInit = true;
                  }
            else
                  boundingRect |= item->boundingRect();

            if (item->note()->selected()) {
                  if (!brsInit) {
                        boundingRectSel = item->boundingRect();
                        brsInit = true;
                        }
                  else
                        boundingRectSel |= item->boundingRect();
                  }

            }

      QRectF viewRect = mapToScene(viewport()->geometry()).boundingRect();

      if (brsInit) {
            horizontalScrollBar()->setValue(boundingRectSel.x());
            verticalScrollBar()->setValue(qMax(boundingRectSel.y() + (boundingRectSel.height() - viewRect.height()) / 2, 0.0));
            }
      else if (brInit) {
            horizontalScrollBar()->setValue(boundingRect.x());
            verticalScrollBar()->setValue(qMax(boundingRect.y() - (boundingRectSel.height() - viewRect.height()) / 2, 0.0));
            }
      else {
            horizontalScrollBar()->setValue(0);
            verticalScrollBar()->setValue(qMax(viewRect.y() - viewRect.height() / 2, 0.0));
            }
      }

//---------------------------------------------------------
//   addChord
//---------------------------------------------------------

void PianoView::addChord(Chord* chrd, int voice)
      {
      for (Chord* c : chrd->graceNotes())
            addChord(c, voice);
      for (Note* note : chrd->notes()) {
            if (note->tieBack())
                  continue;
            _noteList.append(new PianoItem(note, this));
            }
      }

//---------------------------------------------------------
//   updateNotes
//---------------------------------------------------------

void PianoView::updateNotes()
      {
      scene()->blockSignals(true);  // block changeSelection()
      scene()->clearFocus();
      scene()->clear();
      clearNoteData();

      int staffIdx = _staff->idx();
      if (staffIdx == -1)
            return;

      SegmentType st = SegmentType::ChordRest;
      for (Segment* s = _staff->score()->firstSegment(st); s; s = s->next1(st)) {
            for (int voice = 0; voice < VOICES; ++voice) {
                  int track = voice + staffIdx * VOICES;
                  Element* e = s->element(track);
                  if (e && e->isChord())
                        addChord(toChord(e), voice);
                  }
            }
      for (int i = 0; i < 3; ++i)
            moveLocator(i);
      scene()->blockSignals(false);

      scene()->update(sceneRect());
      }

//---------------------------------------------------------
//   updateNotes
//---------------------------------------------------------

void PianoView::clearNoteData()
      {
      for (int i = 0; i < _noteList.size(); ++i)
            delete _noteList[i];

      _noteList.clear();
      }


//---------------------------------------------------------
//   getSelectedItems
//---------------------------------------------------------

QList<PianoItem*> PianoView::getSelectedItems()
      {
      QList<PianoItem*> list;
      for (int i = 0; i < _noteList.size(); ++i) {
            if (_noteList.at(i)->note()->selected())
                  list.append(_noteList[i]);
            }
      return list;
      }

//---------------------------------------------------------
//   getItems
//---------------------------------------------------------

QList<PianoItem*> PianoView::getItems()
      {
      QList<PianoItem*> list;
      for (int i = 0; i < _noteList.size(); ++i)
            list.append(_noteList[i]);
      return list;
      }

//---------------------------------------------------------
//   getAction
//    returns action for shortcut
//---------------------------------------------------------

QAction* PianoView::getAction(const char* id)
      {
      Shortcut* s = Shortcut::getShortcut(id);
      return s ? s->action() : 0;
      }

//---------------------------------------------------------
//   showNoteTweaker
//---------------------------------------------------------

void PianoView::showNoteTweaker()
      {
      emit showNoteTweakerRequest();
      }

//---------------------------------------------------------
//   setXZoom
//---------------------------------------------------------

void PianoView::setXZoom(int value)
      {
      if (_xZoom != value) {
            _xZoom = value;
            scene()->update();
            emit xZoomChanged(_xZoom);
            }
      }

//---------------------------------------------------------
//   setBarPattern
//---------------------------------------------------------

void PianoView::setBarPattern(int value)
      {
      if (_barPattern != value) {
            _barPattern = value;
            scene()->update();
            emit barPatternChanged(_barPattern);
            }
      }

//---------------------------------------------------------
//   setBarPattern
//---------------------------------------------------------

void PianoView::togglePitchHighlight(int pitch)
      {
      _pitchHighlight[pitch] = _pitchHighlight[pitch] ? 0 : 1;
      scene()->update();
      }

//---------------------------------------------------------
//   setSubBeats
//---------------------------------------------------------

void PianoView::setTuplet(int value)
      {
      if (_tuplet != value) {
            _tuplet = value;
            scene()->update();
            emit tupletChanged(_tuplet);
            }
      }

//---------------------------------------------------------
//   setSubdiv
//---------------------------------------------------------

void PianoView::setSubdiv(int value)
      {
      if (_subdiv != value) {
            _subdiv = value;
            scene()->update();
            emit subdivChanged(_subdiv);
            }
      }

}


