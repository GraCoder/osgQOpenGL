#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QColorDialog>
#include <QComboBox>
#include <QLabel>
#include <QGraphicsScene>
#include <QPushButton>
#include <osgQOpenGL/TestWidget>

//============================================================================//
//                                  ColorEdit                                 //
//============================================================================//

ColorEdit::ColorEdit(QRgb initialColor, int id)
: m_color(initialColor), m_id(id)
{
	QHBoxLayout *layout = new QHBoxLayout;
	setLayout(layout);
	layout->setContentsMargins(0, 0, 0, 0);

	m_lineEdit = new QLineEdit(QString::number(m_color, 16));
	layout->addWidget(m_lineEdit);

	m_button = new QFrame;
	QPalette palette = m_button->palette();
	palette.setColor(QPalette::Window, QColor(m_color));
	m_button->setPalette(palette);
	m_button->setAutoFillBackground(true);
	m_button->setMinimumSize(32, 0);
	m_button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
	m_button->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
	layout->addWidget(m_button);

	connect(m_lineEdit, SIGNAL(editingFinished()), this, SLOT(editDone()));
}

void ColorEdit::editDone()
{
	bool ok;
	QRgb newColor = m_lineEdit->text().toUInt(&ok, 16);
	if (ok)
		setColor(newColor);
}

void ColorEdit::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		QColor color(m_color);
		QColorDialog dialog(color, 0);
		dialog.setOption(QColorDialog::ShowAlphaChannel, true);
		// The ifdef block is a workaround for the beta, TODO: remove when bug 238525 is fixed
#ifdef Q_WS_MAC
		dialog.setOption(QColorDialog::DontUseNativeDialog, true);
#endif
		dialog.move(280, 120);
		if (dialog.exec() == QDialog::Rejected)
			return;
		QRgb newColor = dialog.selectedColor().rgba();
		if (newColor == m_color)
			return;
		setColor(newColor);
	}
}

void ColorEdit::setColor(QRgb color)
{
	m_color = color;
	m_lineEdit->setText(QString::number(m_color, 16)); // "Clean up" text
	QPalette palette = m_button->palette();
	palette.setColor(QPalette::Window, QColor(m_color));
	m_button->setPalette(palette);
	emit colorChanged(m_color, m_id);
}

//============================================================================//
//                                  FloatEdit                                 //
//============================================================================//

FloatEdit::FloatEdit(float initialValue, int id)
: m_value(initialValue), m_id(id)
{
	QHBoxLayout *layout = new QHBoxLayout;
	setLayout(layout);
	layout->setContentsMargins(0, 0, 0, 0);

	m_lineEdit = new QLineEdit(QString::number(m_value));
	layout->addWidget(m_lineEdit);

	connect(m_lineEdit, SIGNAL(editingFinished()), this, SLOT(editDone()));
}

void FloatEdit::editDone()
{
	bool ok;
	float newValue = m_lineEdit->text().toFloat(&ok);
	if (ok) {
		m_value = newValue;
		m_lineEdit->setText(QString::number(m_value)); // "Clean up" text
		emit valueChanged(m_value, m_id);
	}
}

//============================================================================//
//                           TwoSidedGraphicsWidget                           //
//============================================================================//

TwoSidedGraphicsWidget::TwoSidedGraphicsWidget(QGraphicsScene *scene)
: QObject(scene)
, m_current(0)
, m_angle(0)
, m_delta(0)
{
	for (int i = 0; i < 2; ++i)
		m_proxyWidgets[i] = 0;
}

void TwoSidedGraphicsWidget::setWidget(int index, QWidget *widget)
{
	if (index < 0 || index >= 2)
	{
		qWarning("TwoSidedGraphicsWidget::setWidget: Index out of bounds, index == %d", index);
		return;
	}

	GraphicsWidget *proxy = new GraphicsWidget;
	proxy->setWidget(widget);

	if (m_proxyWidgets[index])
		delete m_proxyWidgets[index];
	m_proxyWidgets[index] = proxy;

	proxy->setCacheMode(QGraphicsItem::ItemCoordinateCache);
	proxy->setZValue(1e30); // Make sure the dialog is drawn on top of all other (OpenGL) items

	if (index != m_current)
		proxy->setVisible(false);

	qobject_cast<QGraphicsScene *>(parent())->addItem(proxy);
}

QWidget *TwoSidedGraphicsWidget::widget(int index)
{
	if (index < 0 || index >= 2)
	{
		qWarning("TwoSidedGraphicsWidget::widget: Index out of bounds, index == %d", index);
		return 0;
	}
	return m_proxyWidgets[index]->widget();
}

void TwoSidedGraphicsWidget::flip()
{
	m_delta = (m_current == 0 ? 9 : -9);
	animateFlip();
}

void TwoSidedGraphicsWidget::animateFlip()
{
	m_angle += m_delta;
	if (m_angle == 90) {
		int old = m_current;
		m_current ^= 1;
		m_proxyWidgets[old]->setVisible(false);
		m_proxyWidgets[m_current]->setVisible(true);
		m_proxyWidgets[m_current]->setGeometry(m_proxyWidgets[old]->geometry());
	}

	QRectF r = m_proxyWidgets[m_current]->boundingRect();
	m_proxyWidgets[m_current]->setTransform(QTransform()
		.translate(r.width() / 2, r.height() / 2)
		.rotate(m_angle - 180 * m_current, Qt::YAxis)
		.translate(-r.width() / 2, -r.height() / 2));

	if ((m_current == 0 && m_angle > 0) || (m_current == 1 && m_angle < 180))
		QTimer::singleShot(25, this, SLOT(animateFlip()));
}

QVariant GraphicsWidget::itemChange(GraphicsItemChange change, const QVariant &value)
{
	if (change == ItemPositionChange && scene()) {
		QRectF rect = boundingRect();
		QPointF pos = value.toPointF();
		QRectF sceneRect = scene()->sceneRect();
		if (pos.x() + rect.left() < sceneRect.left())
			pos.setX(sceneRect.left() - rect.left());
		else if (pos.x() + rect.right() >= sceneRect.right())
			pos.setX(sceneRect.right() - rect.right());
		if (pos.y() + rect.top() < sceneRect.top())
			pos.setY(sceneRect.top() - rect.top());
		else if (pos.y() + rect.bottom() >= sceneRect.bottom())
			pos.setY(sceneRect.bottom() - rect.bottom());
		return pos;
	}
	return QGraphicsProxyWidget::itemChange(change, value);
}

void GraphicsWidget::resizeEvent(QGraphicsSceneResizeEvent *event)
{
	setCacheMode(QGraphicsItem::NoCache);
	setCacheMode(QGraphicsItem::ItemCoordinateCache);
	QGraphicsProxyWidget::resizeEvent(event);
}

void GraphicsWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	painter->setRenderHint(QPainter::Antialiasing, false);
	QGraphicsProxyWidget::paint(painter, option, widget);
	//painter->setRenderHint(QPainter::Antialiasing, true);
}

//============================================================================//
//                             RenderOptionsDialog                            //
//============================================================================//

RenderOptionsDialog::RenderOptionsDialog()
: QDialog(0, Qt::CustomizeWindowHint | Qt::WindowTitleHint)
{
	setWindowOpacity(0.75);
	setWindowTitle(tr("Options (double click to flip)"));
	QGridLayout *layout = new QGridLayout;
	setLayout(layout);
	layout->setColumnStretch(1, 1);

	int row = 0;

	//QCheckBox *check = new QCheckBox(tr("Dynamic cube map"));
	//check->setCheckState(Qt::Unchecked);
	// Dynamic cube maps are only enabled when multi-texturing and render to texture are available.
	//check->setEnabled(glActiveTexture && glGenFramebuffersEXT);
	//connect(check, SIGNAL(stateChanged(int)), this, SIGNAL(dynamicCubemapToggled(int)));
	//layout->addWidget(check, 0, 0, 1, 2);
	++row;

	QPalette palette;

	// Load all .par files
	// .par files have a simple syntax for specifying user adjustable uniform variables.
	QSet<QByteArray> uniforms;
	QList<QString> filter = QStringList("*.par");
	QList<QFileInfo> files = QDir(":/res/boxes/").entryInfoList(filter, QDir::Files | QDir::Readable);

	foreach (QFileInfo fileInfo, files) {
		QFile file(fileInfo.absoluteFilePath());
		if (file.open(QIODevice::ReadOnly)) {
			while (!file.atEnd()) {
				QList<QByteArray> tokens = file.readLine().simplified().split(' ');
				QList<QByteArray>::const_iterator it = tokens.begin();
				if (it == tokens.end())
					continue;
				QByteArray type = *it;
				if (++it == tokens.end())
					continue;
				QByteArray name = *it;
				bool singleElement = (tokens.size() == 3); // type, name and one value
				char counter[10] = "000000000";
				int counterPos = 8; // position of last digit
				while (++it != tokens.end()) {
					m_parameterNames << name;
					if (!singleElement) {
						m_parameterNames.back() += "[";
						m_parameterNames.back() += counter + counterPos;
						m_parameterNames.back() += "]";
						int j = 8; // position of last digit
						++counter[j];
						while (j > 0 && counter[j] > '9') {
							counter[j] = '0';
							++counter[--j];
						}
						if (j < counterPos)
							counterPos = j;
					}

					if (type == "color") {
						layout->addWidget(new QLabel(m_parameterNames.back()));
						bool ok;
						ColorEdit *colorEdit = new ColorEdit(it->toUInt(&ok, 16), m_parameterNames.size() - 1);
						m_parameterEdits << colorEdit;
						layout->addWidget(colorEdit);
						connect(colorEdit, SIGNAL(colorChanged(QRgb,int)), this, SLOT(setColorParameter(QRgb,int)));
						++row;
					} else if (type == "float") {
						layout->addWidget(new QLabel(m_parameterNames.back()));
						bool ok;
						FloatEdit *floatEdit = new FloatEdit(it->toFloat(&ok), m_parameterNames.size() - 1);
						m_parameterEdits << floatEdit;
						layout->addWidget(floatEdit);
						connect(floatEdit, SIGNAL(valueChanged(float,int)), this, SLOT(setFloatParameter(float,int)));
						++row;
					}
				}
			}
			file.close();
		}
	}

	layout->addWidget(new QLabel(tr("Texture:")));
	m_textureCombo = new QComboBox;
	connect(m_textureCombo, SIGNAL(currentIndexChanged(int)), this, SIGNAL(textureChanged(int)));
	layout->addWidget(m_textureCombo);
	++row;

	layout->addWidget(new QLabel(tr("Shader:")));
	m_shaderCombo = new QComboBox;
	connect(m_shaderCombo, SIGNAL(currentIndexChanged(int)), this, SIGNAL(shaderChanged(int)));
	layout->addWidget(m_shaderCombo);
	++row;

	layout->setRowStretch(row, 1);
}

int RenderOptionsDialog::addTexture(const QString &name)
{
	m_textureCombo->addItem(name);
	return m_textureCombo->count() - 1;
}

int RenderOptionsDialog::addShader(const QString &name)
{
	m_shaderCombo->addItem(name);
	return m_shaderCombo->count() - 1;
}

void RenderOptionsDialog::emitParameterChanged()
{
	foreach (ParameterEdit *edit, m_parameterEdits)
		edit->emitChange();
}

void RenderOptionsDialog::setColorParameter(QRgb color, int id)
{
	emit colorParameterChanged(m_parameterNames[id], color);
}

void RenderOptionsDialog::setFloatParameter(float value, int id)
{
	emit floatParameterChanged(m_parameterNames[id], value);
}

void RenderOptionsDialog::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
		emit doubleClicked();
}

//============================================================================//
//                                 ItemDialog                                 //
//============================================================================//

ItemDialog::ItemDialog()
: QDialog(0, Qt::CustomizeWindowHint | Qt::WindowTitleHint)
{
	setWindowTitle(tr("Items (double click to flip)"));
	setWindowOpacity(0.75);
	resize(160, 100);

	QVBoxLayout *layout = new QVBoxLayout;
	setLayout(layout);
	QPushButton *button;

	button = new QPushButton(tr("Add Qt box"));
	layout->addWidget(button);
	connect(button, SIGNAL(clicked()), this, SLOT(triggerNewQtBox()));

	button = new QPushButton(tr("Add circle"));
	layout->addWidget(button);
	connect(button, SIGNAL(clicked()), this, SLOT(triggerNewCircleItem()));

	button = new QPushButton(tr("Add square"));
	layout->addWidget(button);
	connect(button, SIGNAL(clicked()), this, SLOT(triggerNewSquareItem()));

	layout->addStretch(1);
}

void ItemDialog::triggerNewQtBox()
{
	emit newItemTriggered(QtBoxItem);
}

void ItemDialog::triggerNewCircleItem()
{
	emit newItemTriggered(CircleItem);
}

void ItemDialog::triggerNewSquareItem()
{
	emit newItemTriggered(SquareItem);
}

void ItemDialog::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
		emit doubleClicked();
}