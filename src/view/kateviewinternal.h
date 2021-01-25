/*
    SPDX-FileCopyrightText: 2002-2007 Hamish Rodda <rodda@kde.org>
    SPDX-FileCopyrightText: 2002 John Firebaugh <jfirebaugh@kde.org>
    SPDX-FileCopyrightText: 2002 Joseph Wenninger <jowenn@kde.org>
    SPDX-FileCopyrightText: 2002 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2007 Mirko Stocker <me@misto.ch>

    Based on KWriteView:
    SPDX-FileCopyrightText: 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _KATE_VIEW_INTERNAL_
#define _KATE_VIEW_INTERNAL_

#include <ktexteditor/attribute.h>

#include "inlinenotedata.h"
#include "katedocument.h"
#include "katerenderer.h"
#include "katetextcursor.h"
#include "katetextline.h"
#include "kateview.h"

#include <QDrag>
#include <QElapsedTimer>
#include <QPoint>
#include <QPointer>
#include <QSet>
#include <QTime>
#include <QTimer>
#include <QWidget>

#include <memory>

namespace KTextEditor
{
class MovingRange;
class TextHintProvider;
}

class KateIconBorder;
class KateScrollBar;
class KateAnnotationItemDelegate;
class KateAnnotationGroupPositionState;
class KateTextLayout;
class KateTextAnimation;
class KateAbstractInputMode;
class ZoomEventFilter;

class QScrollBar;

class KateViewInternal : public QWidget
{
    Q_OBJECT

    friend class KTextEditor::ViewPrivate;
    friend class KateIconBorder;
    friend class KateScrollBar;
    friend class KateAnnotationItemDelegate;
    friend class KateAnnotationGroupPositionState;
    friend class CalculatingCursor;
    friend class BoundedCursor;
    friend class WrappingCursor;
    friend class CamelCursor;
    friend class KateAbstractInputMode;
    friend class ::KateTextPreview;

public:
    enum Bias { left = -1, none = 0, right = 1 };

public:
    explicit KateViewInternal(KTextEditor::ViewPrivate *view);
    ~KateViewInternal() override;
    KTextEditor::ViewPrivate *view() const
    {
        return m_view;
    }

    // BEGIN EDIT STUFF
public:
    void editStart();
    void editEnd(int editTagLineStart, int editTagLineEnd, bool tagFrom);

    void editSetCursor(const KTextEditor::Cursor &cursor);

private:
    uint editSessionNumber;
    bool editIsRunning;
    KTextEditor::Cursor editOldCursor;
    KTextEditor::Range editOldSelection;
    // END

    // BEGIN TAG & CLEAR & UPDATE STUFF
public:
    bool tagLine(const KTextEditor::Cursor &virtualCursor);

    bool tagLines(int start, int end, bool realLines = false);
    // cursors not const references as they are manipulated within
    bool tagLines(KTextEditor::Cursor start, KTextEditor::Cursor end, bool realCursors = false);

    bool tagRange(const KTextEditor::Range &range, bool realCursors);

    void tagAll();

    void updateDirty();

    void clear();
    // END

private Q_SLOTS:
    // Updates the view and requests a redraw.
    void updateView(bool changed = false, int viewLinesScrolled = 0);

private:
    void makeVisible(const KTextEditor::Cursor &c, int endCol, bool force = false, bool center = false, bool calledExternally = false);

public:
    // Start Position is a virtual cursor
    KTextEditor::Cursor startPos() const
    {
        return m_startPos;
    }
    int startLine() const
    {
        return m_startPos.line();
    }
    int startX() const
    {
        return m_startX;
    }

    KTextEditor::Cursor endPos() const;
    int endLine() const;

    KateTextLayout yToKateTextLayout(int y) const;

    void prepareForDynWrapChange();
    void dynWrapChanged();

public Q_SLOTS:
    void slotIncFontSizes(qreal step = 1.0);
    void slotDecFontSizes(qreal step = 1.0);
    void slotResetFontSizes();

    void paintCursor();

private Q_SLOTS:
    void scrollLines(int line); // connected to the sliderMoved of the m_lineScroll
    void scrollViewLines(int offset);
    void scrollAction(int action);
    void scrollNextPage();
    void scrollPrevPage();
    void scrollPrevLine();
    void scrollNextLine();
    void scrollColumns(int x); // connected to the valueChanged of the m_columnScroll
    void viewSelectionChanged();

public:
    void cursorPrevChar(bool sel = false);
    void cursorNextChar(bool sel = false);
    void wordPrev(bool sel = false);
    void wordNext(bool sel = false);
    void home(bool sel = false);
    void end(bool sel = false);
    void cursorUp(bool sel = false);
    void cursorDown(bool sel = false);
    void cursorToMatchingBracket(bool sel = false);
    void scrollUp();
    void scrollDown();
    void topOfView(bool sel = false);
    void bottomOfView(bool sel = false);
    void pageUp(bool sel = false, bool half = false);
    void pageDown(bool sel = false, bool half = false);
    void top(bool sel = false);
    void bottom(bool sel = false);
    void top_home(bool sel = false);
    void bottom_end(bool sel = false);

    /**
     * Accessor to the current caret position
     * @return position of the caret as @c KTextEditor::Cursor
     * @see KTextEditor::Cursor
     */
    KTextEditor::Cursor cursorPosition() const
    {
        return m_cursor;
    }

    /**
     * Accessor to the current mouse position
     * @return position of the mouse as @c KTextEditor::Cursor
     * @see KTextEditor::Cursor
     */
    KTextEditor::Cursor mousePosition() const
    {
        return m_mouse;
    }

    QPoint cursorToCoordinate(const KTextEditor::Cursor &cursor, bool realCursor = true, bool includeBorder = true) const;
    // by default, works on coordinates of the whole widget, eg. offsetted by the border
    KTextEditor::Cursor coordinatesToCursor(const QPoint &coord, bool includeBorder = true) const;
    QPoint cursorCoordinates(bool includeBorder = true) const;
    KTextEditor::Cursor findMatchingBracket();

    KateIconBorder *iconBorder() const
    {
        return m_leftBorder;
    }

    // EVENT HANDLING STUFF - IMPORTANT
private:
    void fixDropEvent(QDropEvent *event);

    bool isAcceptableInput(const QKeyEvent *e) const;

protected:
    void hideEvent(QHideEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    bool eventFilter(QObject *obj, QEvent *e) override;
    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void moveEvent(QMoveEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void leaveEvent(QEvent *) override;
    void dragEnterEvent(QDragEnterEvent *) override;
    void dragMoveEvent(QDragMoveEvent *) override;
    void dropEvent(QDropEvent *) override;
    void showEvent(QShowEvent *) override;
    void wheelEvent(QWheelEvent *e) override;
    void focusInEvent(QFocusEvent *) override;
    void focusOutEvent(QFocusEvent *) override;
    void inputMethodEvent(QInputMethodEvent *e) override;

    void contextMenuEvent(QContextMenuEvent *e) override;

private Q_SLOTS:
    void tripleClickTimeout();

Q_SIGNALS:
    // emitted when KateViewInternal is not handling its own URI drops
    void dropEventPass(QDropEvent *);

private Q_SLOTS:
    void slotRegionVisibilityChanged();
    void slotRegionBeginEndAddedRemoved(unsigned int);

private:
    void moveChar(Bias bias, bool sel);
    void moveEdge(Bias bias, bool sel);
    KTextEditor::Cursor maxStartPos(bool changed = false);
    void scrollPos(KTextEditor::Cursor &c, bool force = false, bool calledExternally = false, bool emitSignals = true);
    void scrollLines(int lines, bool sel);

    KTextEditor::Attribute::Ptr attributeAt(const KTextEditor::Cursor &position) const;
    int linesDisplayed() const;

    int lineToY(int viewLine) const;

    void updateSelection(const KTextEditor::Cursor &, bool keepSel);
    void setSelection(const KTextEditor::Range &);
    void moveCursorToSelectionEdge();
    void updateCursor(const KTextEditor::Cursor &newCursor, bool force = false, bool center = false, bool calledExternally = false);
    void updateBracketMarks();
    void beginSelectLine(const QPoint &pos);

    void placeCursor(const QPoint &p, bool keepSelection = false, bool updateSelection = true);
    bool isTargetSelected(const QPoint &p);
    // Returns whether the given range affects the area currently visible in the view
    bool rangeAffectsView(const KTextEditor::Range &range, bool realCursors) const;

    void doDrag();

    KateRenderer *renderer() const;

    KTextEditor::ViewPrivate *m_view;
    class KateIconBorder *m_leftBorder;

    int m_mouseX;
    int m_mouseY;
    int m_scrollX;
    int m_scrollY;

    ZoomEventFilter *m_zoomEventFilter;

    Qt::CursorShape m_mouseCursor;

    Kate::TextCursor m_cursor;
    KTextEditor::Cursor m_mouse;
    KTextEditor::Cursor m_displayCursor;

    bool m_possibleTripleClick;

    // Whether the current completion-item was expanded while the last press of ALT
    bool m_completionItemExpanded;
    QElapsedTimer m_altDownTime;

    // Bracket mark and corresponding decorative ranges
    KTextEditor::MovingRange *m_bm, *m_bmStart, *m_bmEnd;
    KTextEditor::MovingCursor *m_bmLastFlashPos;
    std::unique_ptr<KateTextPreview> m_bmPreview;
    void updateBracketMarkAttributes();

    enum DragState { diNone, diPending, diDragging };

    struct _dragInfo {
        DragState state;
        QPoint start;
        QDrag *dragObject;
    } m_dragInfo;

    //
    // line scrollbar + first visible (virtual) line in the current view
    //
    KateScrollBar *m_lineScroll;
    qreal m_accumulatedScroll = 0.0;
    QWidget *m_dummy;

    // These are now cursors to account for word-wrap.
    // Start Position is a virtual cursor
    Kate::TextCursor m_startPos;
    // Count of lines that are visible behind m_startPos.
    // This does not respect dynamic word wrap, so take it as an approximation.
    uint m_visibleLineCount;

    // This is set to false on resize or scroll (other than that called by makeVisible),
    // so that makeVisible is again called when a key is pressed and the cursor is in the same spot
    bool m_madeVisible;
    bool m_shiftKeyPressed;

    // How many lines to should be kept visible above/below the cursor when possible
    void setAutoCenterLines(int viewLines, bool updateView = true);
    int m_autoCenterLines;
    int m_minLinesVisible;

    //
    // column scrollbar + x position
    //
    QScrollBar *m_columnScroll;
    int m_startX;

    // has selection changed while your mouse or shift key is pressed
    bool m_selChangedByUser;
    KTextEditor::Cursor m_selectAnchor;

    enum SelectionMode { Default = 0, Mouse, Word, Line }; ///< for drag selection.
    uint m_selectionMode;
    // when drag selecting after double/triple click, keep the initial selected
    // word/line independent of direction.
    // They get set in the event of a double click, and is used with mouse move + leftbutton
    KTextEditor::Range m_selectionCached;

    // maximal length of textlines visible from given startLine
    int maxLen(int startLine);

    // are we allowed to scroll columns?
    bool columnScrollingPossible();

    // the same for lines
    bool lineScrollingPossible();

    // returns the maximum X value / col value a cursor can take for a specific line range
    int lineMaxCursorX(const KateTextLayout &line);
    int lineMaxCol(const KateTextLayout &line);

    class KateLayoutCache *cache() const;
    KateLayoutCache *m_layoutCache;

    // convenience methods
    KateTextLayout currentLayout() const;
    KateTextLayout previousLayout() const;
    KateTextLayout nextLayout() const;

    // find the cursor offset by (offset) view lines from a cursor.
    // when keepX is true, the column position will be calculated based on the x
    // position of the specified cursor.
    KTextEditor::Cursor viewLineOffset(const KTextEditor::Cursor &virtualCursor, int offset, bool keepX = false);

    KTextEditor::Cursor toRealCursor(const KTextEditor::Cursor &virtualCursor) const;
    KTextEditor::Cursor toVirtualCursor(const KTextEditor::Cursor &realCursor) const;

    // These variable holds the most recent maximum real & visible column number
    bool m_preserveX;
    int m_preservedX;

    int m_wrapChangeViewLine;
    KTextEditor::Cursor m_cachedMaxStartPos;

    //
    // implementation details for KTextEditor::FlashTextInterface
    //
public:
    void flashChar(const KTextEditor::Cursor &pos, KTextEditor::Attribute::Ptr attribute);
    void showBracketMatchPreview();
    void hideBracketMatchPreview();

private:
    QPointer<KateTextAnimation> m_textAnimation;

private Q_SLOTS:
    void doDragScroll();
    void startDragScroll();
    void stopDragScroll();

private:
    // Timers
    QTimer m_dragScrollTimer;
    QTimer m_scrollTimer;
    QTimer m_cursorTimer;
    QTimer m_textHintTimer;

    static const int s_scrollTime = 30;
    static const int s_scrollMargin = 16;

private Q_SLOTS:
    void scrollTimeout();
    void cursorTimeout();
    void textHintTimeout();

    void documentTextInserted(KTextEditor::Document *document, const KTextEditor::Range &range);
    void documentTextRemoved(KTextEditor::Document *document, const KTextEditor::Range &range, const QString &oldText);

    //
    // KTE::TextHintInterface
    //
public:
    void registerTextHintProvider(KTextEditor::TextHintProvider *provider);
    void unregisterTextHintProvider(KTextEditor::TextHintProvider *provider);
    void setTextHintDelay(int delay);
    int textHintDelay() const;
    bool textHintsEnabled(); // not part of the interface

private:
    QVector<KTextEditor::TextHintProvider *> m_textHintProviders;
    int m_textHintDelay;
    QPoint m_textHintPos;

    //
    // IM input stuff
    //
public:
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;

private:
    KTextEditor::MovingRange *m_imPreeditRange;
    QList<KTextEditor::MovingRange *> m_imPreeditRangeChildren;

private:
    void mouseMoved();
    void cursorMoved();

private:
    inline KTextEditor::DocumentPrivate *doc()
    {
        return m_view->doc();
    }
    inline KTextEditor::DocumentPrivate *doc() const
    {
        return m_view->doc();
    }

    // input modes
private:
    QMap<KTextEditor::View::InputMode, KateAbstractInputMode *> m_inputModes;
    KateAbstractInputMode *m_currentInputMode;

    KateInlineNoteData m_activeInlineNote;
    KateInlineNoteData inlineNoteAt(const QPoint &globalPos) const;
    QRect inlineNoteRect(const KateInlineNoteData &note) const;
};

#endif
