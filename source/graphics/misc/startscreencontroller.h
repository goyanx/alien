#ifndef STARTSCREENCONTROLLER_H
#define STARTSCREENCONTROLLER_H

#include <QGraphicsView>

class QTimer;
class StartScreenController : public QObject
{
    Q_OBJECT
public:
    StartScreenController(QObject *parent);
    ~StartScreenController ();

    void runStartScreen (QGraphicsView* view);

private:
    void setupStartScene (QGraphicsView* view);
    void setupTimer ();

    void saveSceneAndView (QGraphicsView* view);
    void createSceneWithLogo ();

private slots:
    void timeout ();

private:
    void scaleAndDecreaseOpacityOfLogo ();
    bool isLogoTransparent () const;
    void restoreScene ();


private:
    QGraphicsView* _view = 0;
    QGraphicsScene* _savedScene = 0;
    QGraphicsScene* _startScene = 0;
    QGraphicsPixmapItem* _logoItem = 0;
    QMatrix _savedViewMatrix;
    QTimer* _timer = 0;

};

#endif // STARTSCREENCONTROLLER_H
