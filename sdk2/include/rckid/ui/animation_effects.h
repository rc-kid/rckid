#include <rckid/ui/widget.h>
#include <rckid/ui/animation.h>

namespace rckid::ui {

    struct Move {
        Widget * target = nullptr;
        Point from;
        Point to;
        Move(Point from, Point to): from{from}, to{to} {}
        Move(Widget * target, Point from, Point to): target{target}, from{from}, to{to} {}
    };

    inline Animation::Builder operator << (Animation::Builder b, Move m) {
        if (m.target == nullptr) {
            b->setOnUpdate([from = m.from, to = m.to](Widget * w, FixedRatio progress) {
                Coord x = from.x + ((to.x - from.x) * progress);
                Coord y = from.y + ((to.y - from.y) * progress);
                w->setRect(Rect::XYWH(x, y, w->width(), w->height()));
            });
        } else {
            b->setOnUpdate([from = m.from, to = m.to, target = m.target](Widget *, FixedRatio progress) {
                Coord x = from.x + ((to.x - from.x) * progress);
                Coord y = from.y + ((to.y - from.y) * progress);
                target->setRect(Rect::XYWH(x, y, target->width(), target->height()));
            });
        }
        return b;
    }

    struct MoveHorizontally {
        Widget * target = nullptr;
        Coord fromX;
        Coord toX;
        MoveHorizontally(Coord fromX, Coord toX): fromX{fromX}, toX{toX} {}
        MoveHorizontally(Widget * target, Coord fromX, Coord toX): target{target}, fromX{fromX}, toX{toX} {}
    };

    inline Animation::Builder operator << (Animation::Builder b, MoveHorizontally mh) {
        if (mh.target == nullptr) {
            b->setOnUpdate([fromX = mh.fromX, toX = mh.toX](Widget * w, FixedRatio progress) {
                Coord x = fromX + ((toX - fromX) * progress);
                w->setRect(Rect::XYWH(x, w->rect().y, w->width(), w->height()));
            });
        } else {
            b->setOnUpdate([fromX = mh.fromX, toX = mh.toX, target = mh.target](Widget *, FixedRatio progress) {
                Coord x = fromX + ((toX - fromX) * progress);
                target->setRect(Rect::XYWH(x, target->rect().y, target->width(), target->height()));
            });
        }
        return b;
    }

    struct MoveVertically {
        Widget * target = nullptr;
        Coord fromY;
        Coord toY;
        MoveVertically(Coord fromY, Coord toY): fromY{fromY}, toY{toY} {}
        MoveVertically(Widget * target, Coord fromY, Coord toY): target{target}, fromY{fromY}, toY{toY} {}
    };

    inline Animation::Builder operator << (Animation::Builder b, MoveVertically mv) {
        if (mv.target == nullptr) {
            b->setOnUpdate([fromY = mv.fromY, toY = mv.toY](Widget * w, FixedRatio progress) {
                Coord y = fromY + ((toY - fromY) * progress);
                w->setRect(Rect::XYWH(w->rect().x, y, w->width(), w->height()));
            });
        } else {
            b->setOnUpdate([fromY = mv.fromY, toY = mv.toY, target = mv.target](Widget *, FixedRatio progress) {
                Coord y = fromY + ((toY - fromY) * progress);
                target->setRect(Rect::XYWH(target->rect().x, y, target->width(), target->height()));
            });
        }
        return b;
    }

} // namespace rckid::ui