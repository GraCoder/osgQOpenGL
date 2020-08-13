#include <QLabel>
#include <QVBoxLayout>
#include <QGraphicsItem>
#include <osgQOpenGL/GraphicsScene>
#include <osgQOpenGL/TestWidget>

void GraphicsScene::setupScene()
{
	QWidget *dialog = new QWidget;
	dialog->setWindowOpacity(0.8);
	dialog->setWindowTitle("Test");
	dialog->setLayout(new QVBoxLayout);
	dialog->resize(60, 60);
	dialog->move(180, 200);
	dialog->layout()->addWidget(new QLabel(tr("1111111111")));

	//first method add widget to scene with title
	//QGraphicsProxyWidget* proxy = new QGraphicsProxyWidget;
	//proxy->setWindowFlags(Qt::Window);
	//proxy->setWidget(dialog);
	//this->addItem(proxy);

	//second method add widget to scene with title, this is vary important, QGraphicsProxyWidget ignore the window flags
	this->addWidget(dialog, dialog->windowFlags());

	foreach(QGraphicsItem *item, items()) 
	{
		item->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
		item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
	}

	m_renderOptions = new RenderOptionsDialog;
	m_renderOptions->move(20, 120);
	m_renderOptions->resize(m_renderOptions->sizeHint());

	m_itemDialog = new ItemDialog;

	TwoSidedGraphicsWidget *twoSided = new TwoSidedGraphicsWidget(this);
	twoSided->setWidget(0, m_renderOptions);
	twoSided->setWidget(1, m_itemDialog);

	int width = 800;
	int height = 600;
	//addItem(new CircleItem(64, width - 64, height - 64));
	//addItem(new CircleItem(64, width - 64, 64));
	//addItem(new CircleItem(64, 64, height - 64));
	//addItem(new CircleItem(64, 64, 64));

	/*
	addItem(new QtBox(64, width - 50, height - 50));
	addItem(new QtBox(64, width - 50, 50));
	addItem(new QtBox(64, 50, height - 50));
	addItem(new QtBox(64, 50, 50));
	*/
}