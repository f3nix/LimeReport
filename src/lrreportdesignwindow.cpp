/***************************************************************************
 *   This file is part of the Lime Report project                          *
 *   Copyright (C) 2015 by Alexander Arin                                  *
 *   arin_a@bk.ru                                                          *
 *                                                                         *
 **                   GNU General Public License Usage                    **
 *                                                                         *
 *   This library is free software: you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 **                  GNU Lesser General Public License                    **
 *                                                                         *
 *   This library is free software: you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License as        *
 *   published by the Free Software Foundation, either version 3 of the    *
 *   License, or (at your option) any later version.                       *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library.                                      *
 *   If not, see <http://www.gnu.org/licenses/>.                           *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 ****************************************************************************/
#include <QToolBar>
#include <QAction>
#include <QDebug>
#include <QMessageBox>
#include <QToolButton>
#include <QDockWidget>
#include <QStatusBar>
#include <QFileDialog>
#include <QApplication>
#include <QMenuBar>
#include <QCheckBox>
#include <QVBoxLayout>

#include "lrreportdesignwindow.h"
#include "lrbandsmanager.h"
#include "lrobjectinspectorwidget.h"
#include "lrbasedesignobjectmodel.h"
#include "lrreportdesignwidget.h"
#include "lrdatabrowser.h"
#include "lrbasedesignintf.h"
#include "lrpagedesignintf.h"

#include "waitform.h"
#include "lrpreviewreportwindow.h"
#include "serializators/lrstorageintf.h"
#include "serializators/lrxmlreader.h"
#include "objectsbrowser/lrobjectbrowser.h"
#include "lraboutdialog.h"


namespace LimeReport{

ReportDesignWindow* ReportDesignWindow::m_instance=0;

ReportDesignWindow::ReportDesignWindow(ReportEnginePrivate *report, QWidget *parent, QSettings* settings) :
    QMainWindow(parent), m_textAttibutesIsChanging(false), m_settings(settings), m_ownedSettings(false), m_progressDialog(0), m_showProgressDialog(true)
{
    initReportEditor(report);
    createActions();
    createMainMenu();
    createToolBars();
    createObjectInspector();
    createDataWindow();
    createObjectsBrowser();
    m_instance=this;
    m_statusBar=new QStatusBar(this);
    m_lblReportName = new QLabel(report->reportFileName(),this);
    m_statusBar->insertWidget(0,m_lblReportName);
    setStatusBar(m_statusBar);
    setWindowTitle("Lime Report Designer");
    restoreSetting();
}

ReportDesignWindow::~ReportDesignWindow()
{
    m_instance=0;
    delete m_validator;
    if (m_ownedSettings&&m_settings) delete m_settings;
}

void ReportDesignWindow::createActions()
{
    m_newReportAction = new QAction(tr("New Report"),this);
    m_newReportAction->setIcon(QIcon(":/report/images/newReport"));
    m_newReportAction->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_N));
    connect(m_newReportAction,SIGNAL(triggered()),this,SLOT(slotNewReport()));

    m_editModeAction = new QAction(tr("Edit Mode"),this);
    m_editModeAction->setIcon(QIcon(":/report/images/editMode"));
    m_editModeAction->setCheckable(true);
    m_editModeAction->setChecked(true);
    connect(m_editModeAction,SIGNAL(triggered()),this,SLOT(slotEditMode()));

    m_undoAction = new QAction(tr("Undo"),this);
    m_undoAction->setIcon(QIcon(":/report/images/undo"));
    m_undoAction->setEnabled(false);
    m_undoAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Z));
    connect(m_undoAction,SIGNAL(triggered()),this,SLOT(slotUndo()));

    m_redoAction = new QAction(tr("Redo"),this);
    m_redoAction->setIcon(QIcon(":/report/images/redo"));
    m_redoAction->setEnabled(false);
    m_redoAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Z));
    connect(m_redoAction,SIGNAL(triggered()),this,SLOT(slotRedo()));

    m_copyAction = new QAction(tr("Copy"),this);
    m_copyAction->setIcon(QIcon(":/report/images/copy"));
    m_copyAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_C));
    connect(m_copyAction,SIGNAL(triggered()),this,SLOT(slotCopy()));

    m_pasteAction = new QAction(tr("Paste"),this);
    m_pasteAction->setIcon(QIcon(":/report/images/paste"));
    m_pasteAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_V));
    connect(m_pasteAction,SIGNAL(triggered()),this,SLOT(slotPaste()));

    m_cutAction = new QAction(tr("Cut"),this);
    m_cutAction->setIcon(QIcon(":/report/images/cut"));
    m_cutAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_X));
    connect(m_cutAction,SIGNAL(triggered()),this,SLOT(slotCut()));

    m_newTextItemAction = new QAction(tr("Text Item"),this);
    m_newTextItemAction->setIcon(QIcon(":/items/TextItem"));
    m_actionMap.insert("TextItem",m_newTextItemAction);
    connect(m_newTextItemAction,SIGNAL(triggered()),this,SLOT(slotNewTextItem()));

    m_saveReportAction = new QAction(tr("Save Report"),this);
    m_saveReportAction->setIcon(QIcon(":/report/images/save"));
    m_saveReportAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_S));
    connect(m_saveReportAction,SIGNAL(triggered()),this,SLOT(slotSaveReport()));

    m_saveReportAsAction = new QAction(tr("Save Report As"),this);
    m_saveReportAsAction->setIcon(QIcon(":/report/images/saveas"));
    m_saveReportAsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_S));
    connect(m_saveReportAsAction,SIGNAL(triggered()),this,SLOT(slotSaveReportAs()));

    m_loadReportAction = new QAction(tr("Load Report"),this);
    m_loadReportAction->setIcon(QIcon(":/report/images/folder"));
    m_loadReportAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_O));
    connect(m_loadReportAction,SIGNAL(triggered()),this,SLOT(slotLoadReport()));

    m_deleteItemAction = new QAction(tr("Delete item"),this);
    m_deleteItemAction->setShortcut(QKeySequence("Del"));
    m_deleteItemAction->setIcon(QIcon(":/report/images/delete"));
    connect(m_deleteItemAction,SIGNAL(triggered()),this,SLOT(slotDelete()));

    m_zoomInReportAction = new QAction(tr("Zoom In"),this);
    m_zoomInReportAction->setIcon(QIcon(":/report/images/zoomIn"));
    connect(m_zoomInReportAction,SIGNAL(triggered()),this,SLOT(slotZoomIn()));

    m_zoomOutReportAction = new QAction(tr("Zoom Out"),this);
    m_zoomOutReportAction->setIcon(QIcon(":/report/images/zoomOut"));
    connect(m_zoomOutReportAction,SIGNAL(triggered()),this,SLOT(slotZoomOut()));

    m_previewReportAction = new QAction(tr("Render Report"),this);
    m_previewReportAction->setIcon(QIcon(":/report/images/render"));
    m_previewReportAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_P));
    connect(m_previewReportAction,SIGNAL(triggered()),this,SLOT(slotPreviewReport()));

    m_testAction = new QAction("test",this);
    m_testAction->setIcon(QIcon(":/report/images/pin"));
    connect(m_testAction,SIGNAL(triggered()),this,SLOT(slotTest()));

//    m_printReportAction = new QAction(tr("Print Report"),this);
//    m_printReportAction->setIcon(QIcon(":/report/images/print"));
//    m_printReportAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_P));
//    connect(m_printReportAction,SIGNAL(triggered()),this,SLOT(slotPrintReport()));

    m_editLayoutMode = new QAction(tr("Edit layouts mode"),this);
    m_editLayoutMode->setIcon(QIcon(":/report/images/editlayout"));
    m_editLayoutMode->setCheckable(true);
    connect(m_editLayoutMode,SIGNAL(triggered()),this,SLOT(slotEditLayoutMode()));

    m_addHLayout = new QAction(tr("Horizontal layout"),this);
    m_addHLayout->setIcon(QIcon(":/report/images/hlayout"));
    connect(m_addHLayout,SIGNAL(triggered()),this,SLOT(slotHLayout()));

    m_aboutAction = new QAction(tr("About"),this);
    m_aboutAction->setIcon(QIcon(":/report/images/copyright"));
    connect(m_aboutAction,SIGNAL(triggered()),this,SLOT(slotShowAbout()));

}

void ReportDesignWindow::createReportToolBar()
{
    m_reportToolBar = new QToolBar(tr("Report Tools"),this);
    m_reportToolBar->setIconSize(QSize(24,24));
    m_reportToolBar->addAction(m_editModeAction);
    m_reportToolBar->addWidget(m_newBandButton);
    m_reportToolBar->addAction(m_newTextItemAction);
    m_reportToolBar->setObjectName("reportTools");
    createItemsActions();
    m_reportToolBar->addSeparator();
    //m_reportToolBar->addAction(m_editLayoutMode);
    m_reportToolBar->addAction(m_addHLayout);
    m_reportToolBar->addSeparator();
    m_reportToolBar->addAction(m_deleteItemAction);
    addToolBar(Qt::LeftToolBarArea,m_reportToolBar);
}

void ReportDesignWindow::createToolBars()
{
    createBandsButton();

    m_mainToolBar = addToolBar("Main Tools");
    m_mainToolBar->setIconSize(QSize(16,16));
    m_mainToolBar->setAllowedAreas(Qt::LeftToolBarArea | Qt::RightToolBarArea | Qt::TopToolBarArea );
    m_mainToolBar->setFloatable(false);
    m_mainToolBar->setObjectName("mainTools");

    m_mainToolBar->addAction(m_newReportAction);
    m_mainToolBar->addAction(m_saveReportAction);
    m_mainToolBar->addAction(m_loadReportAction);
    m_mainToolBar->addSeparator();

    m_mainToolBar->addAction(m_copyAction);
    m_mainToolBar->addAction(m_pasteAction);
    m_mainToolBar->addAction(m_cutAction);
    m_mainToolBar->addAction(m_undoAction);
    m_mainToolBar->addAction(m_redoAction);
    m_mainToolBar->addSeparator();

    m_mainToolBar->addAction(m_zoomInReportAction);
    m_mainToolBar->addAction(m_zoomOutReportAction); 
    m_mainToolBar->addSeparator();

    m_mainToolBar->addAction(m_previewReportAction);
    //m_mainToolBar->addAction(m_printReportAction);

    m_fontEditorBar = new FontEditorWidget(m_reportDesignWidget,tr("Font"),this);
    m_fontEditorBar->setIconSize(m_mainToolBar->iconSize());
    m_fontEditorBar->setObjectName("fontTools");
    addToolBar(m_fontEditorBar);
    m_textAlignmentEditorBar = new TextAlignmentEditorWidget(m_reportDesignWidget,tr("Text alignment"),this);
    m_textAlignmentEditorBar->setIconSize(m_mainToolBar->iconSize());
    m_textAlignmentEditorBar->setObjectName("textAlignmentTools");
    addToolBar(m_textAlignmentEditorBar);
    m_itemsAlignmentEditorBar = new ItemsAlignmentEditorWidget(m_reportDesignWidget,tr("Items alignment"),this);
    m_itemsAlignmentEditorBar->setIconSize(m_mainToolBar->iconSize());
    m_itemsAlignmentEditorBar->setObjectName("itemsAlignmentTools");
    addToolBar(m_itemsAlignmentEditorBar);
    m_itemsBordersEditorBar = new ItemsBordersEditorWidget(m_reportDesignWidget,tr("Borders"),this);
    m_itemsBordersEditorBar->setIconSize(m_mainToolBar->iconSize());
    m_itemsBordersEditorBar->setObjectName("itemsBorederTools");
    addToolBar(m_itemsBordersEditorBar);

    createReportToolBar();
}

void ReportDesignWindow::createItemsActions()
{
    foreach(ItemAttribs items,DesignElementsFactory::instance().attribsMap().values()){
        if (items.m_tag.compare("Item",Qt::CaseInsensitive)==0){
            QAction* tmpAction = new QAction(items.m_alias,this);
            tmpAction->setWhatsThis(DesignElementsFactory::instance().attribsMap().key(items));
            tmpAction->setIcon(QIcon(":/items/"+tmpAction->whatsThis()));
            connect(tmpAction,SIGNAL(triggered()),this,SLOT(slotItemActionCliked()));
            m_reportToolBar->addAction(tmpAction);
            m_actionMap.insert(tmpAction->whatsThis(),tmpAction);
        }
    }
}

void ReportDesignWindow::createBandsButton()
{
    m_newBandButton = new QToolButton(this);
    m_newBandButton->setPopupMode(QToolButton::InstantPopup);
    m_newBandButton->setIcon(QIcon(":/report/images/addBand"));

    m_bandsAddSignalsMap = new QSignalMapper(this);

    m_newReportHeader=new QAction(QIcon(),tr("Report Header"),this);
    connect(m_newReportHeader,SIGNAL(triggered()),m_bandsAddSignalsMap,SLOT(map()));
    m_bandsAddSignalsMap->setMapping(m_newReportHeader,BandDesignIntf::ReportHeader);
    m_newBandButton->addAction(m_newReportHeader);

    m_newReportFooter=new QAction(QIcon(),tr("Report Footer"),this);
    connect(m_newReportFooter,SIGNAL(triggered()),m_bandsAddSignalsMap,SLOT(map()));
    m_bandsAddSignalsMap->setMapping(m_newReportFooter,BandDesignIntf::ReportFooter);
    m_newBandButton->addAction(m_newReportFooter);

    m_newPageHeader=new QAction(QIcon(),tr("Page Header"),this);
    connect(m_newPageHeader,SIGNAL(triggered()),m_bandsAddSignalsMap,SLOT(map()));
    m_bandsAddSignalsMap->setMapping(m_newPageHeader,BandDesignIntf::PageHeader);
    m_newBandButton->addAction(m_newPageHeader);

    m_newPageFooter=new QAction(QIcon(),tr("Page Footer"),this);
    connect(m_newPageFooter,SIGNAL(triggered()),m_bandsAddSignalsMap,SLOT(map()));
    m_bandsAddSignalsMap->setMapping(m_newPageFooter,BandDesignIntf::PageFooter);
    m_newBandButton->addAction(m_newPageFooter);

    m_newData=new QAction(QIcon(),tr("Data"),this);
    connect(m_newData,SIGNAL(triggered()),m_bandsAddSignalsMap,SLOT(map()));
    m_bandsAddSignalsMap->setMapping(m_newData,BandDesignIntf::Data);
    m_newBandButton->addAction(m_newData);

    m_newSubDetail=new QAction(QIcon(),tr("SubDetail"),this);
    m_newSubDetail->setEnabled(false);
    connect(m_newSubDetail,SIGNAL(triggered()),m_bandsAddSignalsMap,SLOT(map()));
    m_bandsAddSignalsMap->setMapping(m_newSubDetail,BandDesignIntf::SubDetailBand);
    m_newBandButton->addAction(m_newSubDetail);

    m_newSubDetailHeader=new QAction(QIcon(),tr("SubDetailHeader"),this);
    m_newSubDetailHeader->setEnabled(false);
    connect(m_newSubDetailHeader,SIGNAL(triggered()),m_bandsAddSignalsMap,SLOT(map()));
    m_bandsAddSignalsMap->setMapping(m_newSubDetailHeader,BandDesignIntf::SubDetailHeader);
    m_newBandButton->addAction(m_newSubDetailHeader);

    m_newSubDetailFooter=new QAction(QIcon(),tr("SubDetailFooter"),this);
    m_newSubDetailFooter->setEnabled(false);
    connect(m_newSubDetailFooter,SIGNAL(triggered()),m_bandsAddSignalsMap,SLOT(map()));
    m_bandsAddSignalsMap->setMapping(m_newSubDetailFooter,BandDesignIntf::SubDetailFooter);
    m_newBandButton->addAction(m_newSubDetailFooter);

    m_newGroupHeader=new QAction(QIcon(),tr("GroupHeader"),this);
    m_newGroupHeader->setEnabled(false);
    connect(m_newGroupHeader,SIGNAL(triggered()),m_bandsAddSignalsMap,SLOT(map()));
    m_bandsAddSignalsMap->setMapping(m_newGroupHeader,BandDesignIntf::GroupHeader);
    m_newBandButton->addAction(m_newGroupHeader);

    m_newGroupFooter=new QAction(QIcon(),tr("GroupFooter"),this);
    m_newGroupFooter->setEnabled(false);
    connect(m_newGroupFooter,SIGNAL(triggered()),m_bandsAddSignalsMap,SLOT(map()));
    m_bandsAddSignalsMap->setMapping(m_newGroupFooter,BandDesignIntf::GroupFooter);
    m_newBandButton->addAction(m_newGroupFooter);

    connect(m_bandsAddSignalsMap,SIGNAL(mapped(int)),this,SLOT(slotNewBand(int)));
}

void ReportDesignWindow::createMainMenu()
{
    m_fileMenu = menuBar()->addMenu(tr("File"));
    m_fileMenu->addAction(m_newReportAction);
    m_fileMenu->addAction(m_loadReportAction);
    m_fileMenu->addAction(m_saveReportAction);
    m_fileMenu->addAction(m_saveReportAsAction);
    m_fileMenu->addAction(m_previewReportAction);
    //m_fileMenu->addAction(m_printReportAction);
    m_editMenu = menuBar()->addMenu(tr("Edit"));
    m_editMenu->addAction(m_redoAction);
    m_editMenu->addAction(m_undoAction);
    m_editMenu->addAction(m_copyAction);
    m_editMenu->addAction(m_pasteAction);
    m_editMenu->addAction(m_cutAction);
    m_infoMenu = menuBar()->addMenu(tr("Info"));
    m_infoMenu->addAction(m_aboutAction);

}

void ReportDesignWindow::initReportEditor(ReportEnginePrivate* report)
{
    m_reportDesignWidget=new ReportDesignWidget(report,this,this);
    setCentralWidget(m_reportDesignWidget);
    connect(m_reportDesignWidget, SIGNAL(itemSelected(LimeReport::BaseDesignIntf*)),
            this, SLOT(slotItemSelected(LimeReport::BaseDesignIntf*)));
    connect(m_reportDesignWidget, SIGNAL(itemPropertyChanged(QString,QString,QVariant,QVariant)),
            this, SLOT(slotItemPropertyChanged(QString,QString,QVariant,QVariant)));
    connect(m_reportDesignWidget, SIGNAL(insertModeStarted()), this, SLOT(slotInsertModeStarted()));

    connect(m_reportDesignWidget, SIGNAL(multiItemSelected()), this, SLOT(slotMultiItemSelected()));
    connect(m_reportDesignWidget, SIGNAL(itemInserted(LimeReport::PageDesignIntf*,QPointF,QString)),
            this, SLOT(slotItemInserted(LimeReport::PageDesignIntf*,QPointF,QString)));
    connect(m_reportDesignWidget, SIGNAL(itemInsertCanceled(QString)), this, SLOT(slotItemInsertCanceled(QString)));
    connect(m_reportDesignWidget->report(),SIGNAL(datasourceCollectionLoadFinished(QString)), this, SLOT(slotUpdateDataBrowser(QString)));
    connect(m_reportDesignWidget, SIGNAL(commandHistoryChanged()),this,SLOT(slotCommandHistoryChanged()));
    connect(m_reportDesignWidget, SIGNAL(bandAdded(LimeReport::PageDesignIntf*,LimeReport::BandDesignIntf*)),
            this, SLOT(slotBandAdded(LimeReport::PageDesignIntf*,LimeReport::BandDesignIntf*)));
    connect(m_reportDesignWidget, SIGNAL(bandDeleted(LimeReport::PageDesignIntf*,LimeReport::BandDesignIntf*)),
            this, SLOT(slotBandDeleted(LimeReport::PageDesignIntf*,LimeReport::BandDesignIntf*)));
    connect(m_reportDesignWidget->report(), SIGNAL(renderStarted()), this, SLOT(renderStarted()));
    connect(m_reportDesignWidget->report(), SIGNAL(renderPageFinished(int)), this, SLOT(renderPageFinished(int)));
    connect(m_reportDesignWidget->report(), SIGNAL(renderFinished()), this, SLOT(renderFinished()));
}

void ReportDesignWindow::createObjectInspector()
{
    m_objectInspector = new ObjectInspectorWidget(this);
    m_propertyModel = new BaseDesignPropertyModel(this);
    m_validator = new ObjectNameValidator();
    m_propertyModel->setValidator(m_validator);
    m_propertyModel->setSubclassesAsLevel(false);
//    connect(m_propertyModel,SIGNAL(objectPropetyChanged(QString,QVariant,QVariant)),this,SLOT(slotItemDataChanged(QString,QVariant,QVariant)));
    m_objectInspector->setModel(m_propertyModel);
    m_objectInspector->setAlternatingRowColors(true);
    m_objectInspector->setRootIsDecorated(!m_propertyModel->subclassesAsLevel());

    QDockWidget *objectDoc = new QDockWidget(this);
    QWidget* w = new QWidget(objectDoc);
    QVBoxLayout* l = new QVBoxLayout(w);
    l->addWidget(m_objectInspector);
    l->setContentsMargins(2,2,2,2);
    w->setLayout(l);
    objectDoc->setWindowTitle(tr("Object Inspector"));
    objectDoc->setWidget(w);
    objectDoc->setObjectName("objectInspector");
    addDockWidget(Qt::LeftDockWidgetArea,objectDoc);
}

void ReportDesignWindow::createObjectsBrowser()
{
    QDockWidget *doc = new QDockWidget(this);
    doc->setWindowTitle(tr("Report structure"));
    m_objectsBrowser = new ObjectBrowser(doc);
    doc->setWidget(m_objectsBrowser);
    doc->setObjectName("structureDoc");
    addDockWidget(Qt::RightDockWidgetArea,doc);
    m_objectsBrowser->setMainWindow(this);
    m_objectsBrowser->setReportEditor(m_reportDesignWidget);
}

void ReportDesignWindow::createDataWindow()
{
    QDockWidget *dataDoc = new QDockWidget(this);
    dataDoc->setWindowTitle(tr("Data Browser"));
    m_dataBrowser=new DataBrowser(dataDoc);
    dataDoc->setWidget(m_dataBrowser);
    dataDoc->setObjectName("dataDoc");
    addDockWidget(Qt::LeftDockWidgetArea,dataDoc);
    m_dataBrowser->setSettings(settings());
    m_dataBrowser->setMainWindow(this);
    m_dataBrowser->setReportEditor(m_reportDesignWidget);
}

void ReportDesignWindow::updateRedoUndo()
{
    m_undoAction->setEnabled(m_reportDesignWidget->isCanUndo());
    m_redoAction->setEnabled(m_reportDesignWidget->isCanRedo());
}

void ReportDesignWindow::startNewReport()
{
    m_reportDesignWidget->clear();
    m_reportDesignWidget->createStartPage();
    m_lblReportName->setText("");
    updateRedoUndo();
    m_newPageHeader->setEnabled(true);
    m_newPageFooter->setEnabled(true);
    m_newReportHeader->setEnabled(true);
    m_newReportFooter->setEnabled(true);
}

void ReportDesignWindow::writePosition()
{
    settings()->beginGroup("DesignerWindow");
    settings()->setValue("Geometry",saveGeometry());
    settings()->setValue("State",saveState());
    settings()->endGroup();
}

void ReportDesignWindow::writeState()
{
    settings()->beginGroup("DesignerWindow");
    settings()->setValue("State",saveState());
    settings()->setValue("InspectorFirsColumnWidth",m_objectInspector->columnWidth(0));
    settings()->endGroup();
}

void ReportDesignWindow::restoreSetting()
{
    settings()->beginGroup("DesignerWindow");
    QVariant v = settings()->value("Geometry");
    if (v.isValid()){
        restoreGeometry(v.toByteArray());
    }
    v = settings()->value("State");
    if (v.isValid()){
        restoreState(v.toByteArray());
    }
    v = settings()->value("InspectorFirsColumnWidth");
    if (v.isValid()){
        m_objectInspector->setColumnWidth(0,v.toInt());
    }
    settings()->endGroup();
}

bool ReportDesignWindow::checkNeedToSave()
{
    if (m_reportDesignWidget->isNeedToSave()){
        QMessageBox::StandardButton button = QMessageBox::question(
            this,"",tr("Report has been modified ! Do you want save the report ?"),
            QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel, QMessageBox::Yes
        );
        switch (button) {
        case QMessageBox::Cancel:
            break;
        case QMessageBox::Yes:
            if (!m_reportDesignWidget->save()) break;
        default:
            return true;
        }
        return false;
    }
    return true;
}

void ReportDesignWindow::showModal()
{
    bool deleteOnClose = testAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_DeleteOnClose,false);
    setAttribute(Qt::WA_ShowModal,true);
    restoreSetting();
    show();
    m_eventLoop.exec();
    if (deleteOnClose) delete this;
}

void ReportDesignWindow::showNonModal()
{
    restoreSetting();
    show();
}

void ReportDesignWindow::setSettings(QSettings* value)
{
    if (m_ownedSettings)
        delete m_settings;
    m_settings=value;
    m_ownedSettings=false;
    restoreSetting();
}

QSettings*ReportDesignWindow::settings()
{
    if (m_settings){
        return m_settings;
    } else {
        m_settings = new QSettings("LimeReport",QApplication::applicationName());
        m_ownedSettings = true;
        return m_settings;
    }
}

void ReportDesignWindow::slotNewReport()
{    
    if (checkNeedToSave()) startNewReport();
}

void ReportDesignWindow::slotNewTextItem()
{
    if (m_newTextItemAction->isChecked()) {m_newTextItemAction->setCheckable(false);return;}
    if (m_reportDesignWidget) {
        m_reportDesignWidget->startInsertMode("TextItem");
        m_newTextItemAction->setCheckable(true);
        m_newTextItemAction->setChecked(true);
    }
}

void ReportDesignWindow::slotNewBand(const QString& bandType)
{
    if(m_reportDesignWidget) m_reportDesignWidget->addBand(bandType);
}

void ReportDesignWindow::slotNewBand(int bandType)
{
    if(m_reportDesignWidget) m_reportDesignWidget->addBand(BandDesignIntf::BandsType(bandType));
}

void ReportDesignWindow::slotItemSelected(LimeReport::BaseDesignIntf *item)
{
    if (m_propertyModel->currentObject()!=item){

        m_newSubDetail->setEnabled(false);
        m_newSubDetailHeader->setEnabled(false);
        m_newSubDetailFooter->setEnabled(false);
        m_newGroupHeader->setEnabled(false);
        m_newGroupFooter->setEnabled(false);

        m_objectInspector->commitActiveEditorData();
        m_propertyModel->setObject(item);
        if (m_propertyModel->subclassesAsLevel())
          m_objectInspector->expandToDepth(0);

        QSet<BandDesignIntf::BandsType> bs;
        bs<<BandDesignIntf::Data<<BandDesignIntf::SubDetailBand;
        BandDesignIntf* band = dynamic_cast<BandDesignIntf*>(item);
        if (band){
            if (bs.contains(band->bandType())){
                m_newSubDetail->setEnabled(true);
            }
            if ((band->bandType()==BandDesignIntf::Data)||
                (band->bandType()==BandDesignIntf::SubDetailBand)
               )
            {
                m_newGroupHeader->setEnabled(true);
            }
            if (band->bandType()==BandDesignIntf::GroupHeader){
                m_newGroupFooter->setEnabled(!band->isConnectedToBand(BandDesignIntf::GroupFooter));
            }
            if (band->bandType()==BandDesignIntf::SubDetailBand){
                m_newSubDetailHeader->setEnabled(!band->isConnectedToBand(BandDesignIntf::SubDetailHeader));
                m_newSubDetailFooter->setEnabled(!band->isConnectedToBand(BandDesignIntf::SubDetailFooter));
            }
        }

        m_fontEditorBar->setItem(item);
        m_textAlignmentEditorBar->setItem(item);
        m_itemsBordersEditorBar->setItem(item);
    }
}

void ReportDesignWindow::slotItemPropertyChanged(const QString &objectName, const QString &propertyName, const QVariant& oldValue, const QVariant& newValue )
{
    Q_UNUSED(oldValue)
    Q_UNUSED(newValue)

    if (m_propertyModel->currentObject()&&(m_propertyModel->currentObject()->objectName()==objectName)){
        m_propertyModel->updateProperty(propertyName);
    }
}

void ReportDesignWindow::slotMultiItemSelected()
{
    m_objectInspector->commitActiveEditorData();

    QList<QObject*> selectionList;
    foreach (QGraphicsItem* gi, m_reportDesignWidget->activePage()->selectedItems()) {
        QObject* oi = dynamic_cast<QObject*>(gi);
        if (oi) selectionList.append(oi);
    }
    m_propertyModel->setMultiObjects(&selectionList);
    if (m_propertyModel->subclassesAsLevel())
       m_objectInspector->expandToDepth(0);
}

void ReportDesignWindow::slotInsertModeStarted()
{
    m_editModeAction->setChecked(false);
}

void ReportDesignWindow::slotItemInserted(PageDesignIntf *, QPointF, const QString &ItemType)
{
    m_editModeAction->setChecked(true);
    if (m_actionMap.value(ItemType))
        m_actionMap.value(ItemType)->setCheckable(false);
}

void ReportDesignWindow::slotItemInsertCanceled(const QString &ItemType)
{
    m_editModeAction->setChecked(true);
    if (m_actionMap.value(ItemType))
        m_actionMap.value(ItemType)->setCheckable(false);
}

void ReportDesignWindow::slotUpdateDataBrowser(const QString &collectionName)
{
    if (collectionName.compare("connections",Qt::CaseInsensitive)==0){
        if (m_dataBrowser) m_dataBrowser->initConnections();
    }
    if (collectionName.compare("queries",Qt::CaseInsensitive)==0){
        if (m_dataBrowser) m_dataBrowser->updateDataTree();
    }
    if (collectionName.compare("subqueries",Qt::CaseInsensitive)==0){
        if (m_dataBrowser) m_dataBrowser->updateDataTree();
    }
    if (collectionName.compare("subproxies",Qt::CaseInsensitive)==0){
        if (m_dataBrowser) m_dataBrowser->updateDataTree();
    }
    if (collectionName.compare("variables",Qt::CaseInsensitive)==0){
        if (m_dataBrowser) m_dataBrowser->updateVariablesTree();
    }
}

void ReportDesignWindow::slotCommandHistoryChanged()
{
    updateRedoUndo();
}

void ReportDesignWindow::slotSaveReport()
{
    m_reportDesignWidget->save();
    m_lblReportName->setText(m_reportDesignWidget->reportFileName());
}

void ReportDesignWindow::slotSaveReportAs()
{
    QString fileName = QFileDialog::getSaveFileName(this,tr("Report file name"),"","Report files(*.lrxml);; All files(*)");
    if (!fileName.isEmpty()){
        m_reportDesignWidget->saveToFile(fileName);
        m_lblReportName->setText(m_reportDesignWidget->reportFileName());
    }
}

void ReportDesignWindow::slotLoadReport()
{
    if (checkNeedToSave()){
        QString fileName = QFileDialog::getOpenFileName(this,tr("Report file name"),"","Report files(*.lrxml);; All files(*)");
        if (!fileName.isEmpty()) {
            QApplication::processEvents();
            setCursor(Qt::WaitCursor);
            m_reportDesignWidget->clear();
            m_reportDesignWidget->loadFromFile(fileName);
            m_lblReportName->setText(fileName);
            m_propertyModel->setObject(0);
            updateRedoUndo();
            unsetCursor();
        }
    }
}

void ReportDesignWindow::slotZoomIn()
{
    m_reportDesignWidget->scale(1.2,1.2);
}

void ReportDesignWindow::slotZoomOut()
{
    m_reportDesignWidget->scale(1/1.2,1/1.2);
}

void ReportDesignWindow::slotEditMode()
{
    m_editModeAction->setChecked(true);
    m_reportDesignWidget->startEditMode();
}

void ReportDesignWindow::slotUndo()
{
    m_reportDesignWidget->undo();
    updateRedoUndo();

}

void ReportDesignWindow::slotRedo()
{
    m_reportDesignWidget->redo();
    updateRedoUndo();
}

void ReportDesignWindow::slotCopy()
{
    m_reportDesignWidget->copy();
}

void ReportDesignWindow::slotPaste()
{
    m_reportDesignWidget->paste();
}

void ReportDesignWindow::slotCut()
{
    m_reportDesignWidget->cut();
}

void ReportDesignWindow::slotDelete()
{
    m_reportDesignWidget->deleteSelectedItems();
}

void ReportDesignWindow::slotEditLayoutMode()
{
    m_reportDesignWidget->editLayoutMode(m_editLayoutMode->isChecked());
}

void ReportDesignWindow::slotHLayout()
{
    m_reportDesignWidget->addHLayout();
}

void ReportDesignWindow::slotTest()
{
}

void ReportDesignWindow::slotPrintReport()
{
    setCursor(Qt::WaitCursor);
    m_reportDesignWidget->report()->printReport();
    setCursor(Qt::ArrowCursor);
}

void ReportDesignWindow::slotPreviewReport()
{
    m_reportDesignWidget->report()->previewReport();
}

void ReportDesignWindow::slotItemActionCliked()
{
    QAction* action=dynamic_cast<QAction*>(sender());
    action->setCheckable(true);
    action->setChecked(true);
    m_reportDesignWidget->startInsertMode(action->whatsThis());
}

void ReportDesignWindow::slotBandAdded(PageDesignIntf *, BandDesignIntf * band)
{
    if (band->isUnique()){
        switch (band->bandType()) {
        case BandDesignIntf::PageHeader:
            m_newPageHeader->setDisabled(true);
            break;
        case BandDesignIntf::PageFooter:
            m_newPageFooter->setDisabled(true);
            break;
        case BandDesignIntf::ReportHeader:
            m_newReportHeader->setDisabled(true);
            break;
        case BandDesignIntf::ReportFooter:
            m_newReportFooter->setDisabled(true);
        default:
            break;
        }
    }
}

void ReportDesignWindow::slotBandDeleted(PageDesignIntf *, BandDesignIntf *band)
{
    if (band->isUnique()){
        switch (band->bandType()) {
        case BandDesignIntf::PageHeader:
            m_newPageHeader->setEnabled(true);
            break;
        case BandDesignIntf::PageFooter:
            m_newPageFooter->setEnabled(true);
            break;
        case BandDesignIntf::ReportHeader:
            m_newReportHeader->setEnabled(true);
            break;
        case BandDesignIntf::ReportFooter:
            m_newReportFooter->setEnabled(true);
        default:
            break;
        }
    }
}

void ReportDesignWindow::renderStarted()
{
    if (m_showProgressDialog){
        m_progressDialog = new QProgressDialog(tr("Rendering report"),tr("Abort"),0,0,this);
        m_progressDialog->open(m_reportDesignWidget->report(),SLOT(cancelRender()));
        QApplication::processEvents();
    }
}

void ReportDesignWindow::renderPageFinished(int renderedPageCount)
{
    if (m_progressDialog)
        m_progressDialog->setLabelText(QString::number(renderedPageCount)+tr(" page rendered"));
}

void ReportDesignWindow::renderFinished()
{
    if (m_progressDialog){
        m_progressDialog->close();
        delete m_progressDialog;
    }
    m_progressDialog = 0;
}

void ReportDesignWindow::slotShowAbout()
{
    AboutDialog* about = new AboutDialog(this);
    about->exec();
}

void ReportDesignWindow::closeEvent(QCloseEvent * event)
{
    if (checkNeedToSave()){    
        m_dataBrowser->closeAllDataWindows();
        writeState();
#ifdef Q_OS_WIN
        writePosition();
#endif
#ifdef Q_OS_MAC
        writePosition();
#endif
        m_eventLoop.exit();
        event->accept();
    } else event->ignore();
}

void ReportDesignWindow::resizeEvent(QResizeEvent*)
{
#ifdef Q_OS_UNIX
    writePosition();
#endif
}

void ReportDesignWindow::moveEvent(QMoveEvent*)
{
#ifdef Q_OS_UNIX
    writePosition();
#endif
}

bool ObjectNameValidator::validate(const QString &propName, const QVariant &propValue, QObject *object, QString &msg)
{
    if (propName.compare("objectName")==0){
        BaseDesignIntf* bd = dynamic_cast<BaseDesignIntf*>(object);
        if (bd){
            if (bd->page()->reportItemByName(propValue.toString())){
                msg = QString(QObject::tr("Object with name %1 already exists").arg(propValue.toString()));
                return false;
            } else (bd->emitObjectNamePropertyChanged(object->objectName(),propValue.toString()));
        }
    }
    return true;
}

}
