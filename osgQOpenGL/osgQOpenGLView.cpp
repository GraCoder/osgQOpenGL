#include <osgQOpenGL/osgQOpenGLView>
#include <osgQOpenGL/OSGRenderer>
#include <osgQOpenGL/GraphicsScene>

#include <osgViewer/Viewer>
#include <osg/GL>

#include <QApplication>
#include <QOpenGLWidget>
#include <QKeyEvent>
#include <QInputDialog>
#include <QLayout>
#include <QMainWindow>
#include <QScreen>
#include <QWindow>
#include <QDebug>


class VOpenGLWidget : public QOpenGLWidget 
{
public:
	VOpenGLWidget(QWidget *parent = nullptr)
		: QOpenGLWidget(parent)
	{
	}
	void initializeGL()
	{
		auto wgt = (osgQOpenGLView *)parent();
		wgt->initializeGL();
	}
};


osgQOpenGLView::osgQOpenGLView(QWidget* parent)
    : QGraphicsView(parent)
{
	init();
}

osgQOpenGLView::osgQOpenGLView(osg::ArgumentParser* arguments,
                                   QWidget* parent) :
    QGraphicsView(parent),
    _arguments(arguments)
{
	init();
}

osgQOpenGLView::~osgQOpenGLView()
{
}

osgViewer::Viewer* osgQOpenGLView::getOsgViewer()
{
    return m_renderer;
}

OpenThreads::ReadWriteMutex* osgQOpenGLView::mutex()
{
    return &_osgMutex;
}


void osgQOpenGLView::initializeGL()
{
    initializeOpenGLFunctions();
    createRenderer();
    emit initialized();
}

void osgQOpenGLView::resizeGL(int w, int h)
{
	auto wgt = (QOpenGLWidget *)viewport();
	if (wgt) wgt->makeCurrent();
	//if(m_renderer == nullptr)
	//	initializeGL();
    QScreen* screen = windowHandle()
                      && windowHandle()->screen() ? windowHandle()->screen() :
                      qApp->screens().front();
	if(m_renderer)
		m_renderer->resize(w, h, screen->devicePixelRatio());
}

void osgQOpenGLView::paintGL()
{
    OpenThreads::ScopedReadLock locker(_osgMutex);
	if (_isFirstFrame) {
		_isFirstFrame = false;
		auto wgt = (QOpenGLWidget*)viewport();
		m_renderer->getCamera()->getGraphicsContext()
			->setDefaultFboId(wgt->defaultFramebufferObject());
	}
	m_renderer->frame();
}

void osgQOpenGLView::resizeEvent(QResizeEvent * event)
{
	auto &size = event->size();
	if (scene())
		scene()->setSceneRect(QRect(QPoint(0, 0), size));

	QGraphicsView::resizeEvent(event);

	auto wgt = (QOpenGLWidget *)viewport();
	wgt->makeCurrent();
	resizeGL(size.width(), size.height());
}

void osgQOpenGLView::keyPressEvent(QKeyEvent* event)
{
    Q_ASSERT(m_renderer);

    if(event->key() == Qt::Key_F)
    {
        static QSize g;
        static QMargins sMargins;

        if(parent() && parent()->isWidgetType())
        {
            QMainWindow* _mainwindow = dynamic_cast<QMainWindow*>(parent());

            if(_mainwindow)
            {
                g = size();

                if(layout())
                    sMargins = layout()->contentsMargins();

                bool ok = true;

                // select screen
                if(qApp->screens().size() > 1)
                {
                    QMap<QString, QScreen*> screens;
                    int screenNumber = 0;

                    for(QScreen* screen : qApp->screens())
                    {
                        QString name = screen->name();

                        if(name.isEmpty())
                        {
                            name = tr("Screen %1").arg(screenNumber);
                        }

                        name += " (" + QString::number(screen->size().width()) + "x" + QString::number(
                                    screen->size().height()) + ")";
                        screens[name] = screen;
                        ++screenNumber;
                    }

                    QString selected = QInputDialog::getItem(this,
                                                             tr("Choose fullscreen target screen"), tr("Screen"), screens.keys(), 0, false,
                                                             &ok);

                    if(ok && !selected.isEmpty())
                    {
						auto wgt = (QOpenGLWidget *)viewport();
						wgt-> context()->setScreen(screens[selected]);
                        move(screens[selected]->geometry().x(), screens[selected]->geometry().y());
                        resize(screens[selected]->geometry().width(),
                               screens[selected]->geometry().height());
                    }
                }

                if(ok)
                {
                    // in fullscreen mode, a thiner (1px) border around
                    // viewer widget
                    if(layout())
                        layout()->setContentsMargins(1, 1, 1, 1);

                    setParent(0);
                    showFullScreen();
                }
            }
        }
        else
        {
            showNormal();
            setMinimumSize(g);
            QMainWindow* _mainwindow = dynamic_cast<QMainWindow*>(parent());
            _mainwindow->setCentralWidget(this);

            if(layout())
                layout()->setContentsMargins(sMargins);

            qApp->processEvents();
            setMinimumSize(QSize(1, 1));
        }
    }
    else // not 'F' key
    {
        // forward event to renderer
        m_renderer->keyPressEvent(event);
    }
}

void osgQOpenGLView::keyReleaseEvent(QKeyEvent* event)
{
    Q_ASSERT(m_renderer);
    // forward event to renderer
    m_renderer->keyReleaseEvent(event);
}

void osgQOpenGLView::mousePressEvent(QMouseEvent* event)
{
	QGraphicsView::mousePressEvent(event);

	if (event->isAccepted())
		return;

	Q_ASSERT(m_renderer);
	// forward event to renderer
	m_renderer->mousePressEvent(event);
}

void osgQOpenGLView::mouseReleaseEvent(QMouseEvent* event)
{
    Q_ASSERT(m_renderer);
    // forward event to renderer
    m_renderer->mouseReleaseEvent(event);

	QGraphicsView::mouseReleaseEvent(event) ;
}

void osgQOpenGLView::mouseDoubleClickEvent(QMouseEvent* event)
{
    Q_ASSERT(m_renderer);
    // forward event to renderer
    m_renderer->mouseDoubleClickEvent(event);

	QGraphicsView::mouseDoubleClickEvent(event);
}

void osgQOpenGLView::mouseMoveEvent(QMouseEvent* event)
{
	QGraphicsView::mouseMoveEvent(event);

	if (event->isAccepted())
		return;

    Q_ASSERT(m_renderer);
    // forward event to renderer
    m_renderer->mouseMoveEvent(event);
}

void osgQOpenGLView::wheelEvent(QWheelEvent* event)
{
    Q_ASSERT(m_renderer);
    // forward event to renderer
    m_renderer->wheelEvent(event);

	QGraphicsView::wheelEvent(event);
}

void osgQOpenGLView::setDefaultDisplaySettings()
{
    osg::DisplaySettings* ds = osg::DisplaySettings::instance().get();
    ds->setNvOptimusEnablement(1);
    ds->setStereo(false);
}

void osgQOpenGLView::init()
{
	QOpenGLWidget *viewport = new VOpenGLWidget(this);
	viewport->setMouseTracking(true);
    // Initializes OpenGL function resolution for the current context.
	setViewport(viewport);
	viewport->setMinimumSize(1, 1);
	setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

	auto sc = new GraphicsScene();
	sc->setupScene();
	setScene(sc);
}

void osgQOpenGLView::createRenderer()
{
    // call this before creating a View...
    setDefaultDisplaySettings();
	if (!_arguments) {
		m_renderer = new OSGRenderer(this, enQGLView);
	} else {
		m_renderer = new OSGRenderer(_arguments, this, enQGLView);
	}
	QScreen* screen = windowHandle()
                      && windowHandle()->screen() ? windowHandle()->screen() :
                      qApp->screens().front();
    m_renderer->setupOSG(width(), height(), screen->devicePixelRatio());
}

void osgQOpenGLView::drawBackground(QPainter * painter, const QRectF & rect)
{
	painter->save();
	painter->beginNativePainting();
	paintGL();
	painter->endNativePainting();
	painter->restore();
}
