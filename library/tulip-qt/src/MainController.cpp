#include "tulip/MainController.h"

#include <stdio.h>

#include <QtGui/QDockWidget>
#include <QtGui/QWorkspace>
#include <QtGui/QToolBar>
#include <QtGui/QMenuBar>
#include <QtGui/QMessageBox>
#include <QtGui/QStatusBar>
#include <QtGui/QInputDialog>
#include <QtGui/QClipboard>
#include <QtGui/QTabWidget>

#include <tulip/tuliphash.h>
#include <tulip/Graph.h>
#include <tulip/Algorithm.h>
#include <tulip/BooleanProperty.h>
#include <tulip/ColorProperty.h>
#include <tulip/DoubleProperty.h>
#include <tulip/IntegerProperty.h>
#include <tulip/LayoutProperty.h>
#include <tulip/SizeProperty.h>
#include <tulip/StringProperty.h>
#include <tulip/TlpQtTools.h>
#include <tulip/ExtendedClusterOperation.h>
#include <tulip/StableIterator.h>
#include <tulip/ForEach.h>
#include <tulip/DrawingTools.h>

#include "tulip/ControllerAlgorithmTools.h"
#include "tulip/ControllerViewsTools.h"
#include "tulip/TabWidget.h"
#include "tulip/ViewPluginsManager.h"
#include "tulip/QtProgress.h"
#include "tulip/Morphing.h"
#include "tulip/FindSelectionWidget.h"
#include "tulip/NodeLinkDiagramComponent.h"
#include "tulip/GlMainWidget.h"
#include "tulip/InteractorManager.h"

using namespace std;

namespace tlp {

  //**********************************************************************
  static Graph* getCurrentSubGraph(Graph *graph,const string &name) {
    if(graph->getAttribute<string>("name")==name)
      return graph;

    Graph *sg;
    forEach(sg, graph->getSubGraphs()) {
      Graph *csg = getCurrentSubGraph(sg, name);
      if (csg)
        returnForEach(csg);
    }
    return (Graph *) 0;
  }
  //*********************************************************************
  static std::vector<std::string> getItemGroupNames(std::string itemGroup) {
    std::string::size_type start = 0;
    std::string::size_type end = 0;
    std::vector<std::string> groupNames;
    const char * separator = "::";

    while(true) {
      start = itemGroup.find_first_not_of(separator, end);
      if (start == std::string::npos) {
	return groupNames;
      }
      end = itemGroup.find_first_of(separator, start);
      if (end == std::string::npos)
	end = itemGroup.length();
      groupNames.push_back(itemGroup.substr(start, end - start));
    }
  }
  //**********************************************************************
  static void insertInMenu(QMenu &menu, string itemName, string itemGroup,
			   std::vector<QMenu*> &groupMenus, std::string::size_type &nGroups,
			   QObject *receiver, const char *slot) {
    std::vector<std::string> itemGroupNames = getItemGroupNames(itemGroup);
    QMenu *subMenu = &menu;
    std::string::size_type nGroupNames = itemGroupNames.size();
    for (std::string::size_type i = 0; i < nGroupNames; i++) {
      QMenu *groupMenu = (QMenu *) 0;
      for (std::string::size_type j = 0; j < nGroups; j++) {
	if (itemGroupNames[i] == groupMenus[j]->objectName().toUtf8().data()) {
	  subMenu = groupMenu = groupMenus[j];
	  break;
	}
      }
      if (!groupMenu) {
	groupMenu = new QMenu(itemGroupNames[i].c_str(), subMenu);
	groupMenu->setObjectName(QString(itemGroupNames[i].c_str()));
	subMenu->addMenu(groupMenu);
	groupMenus.push_back(groupMenu);
	nGroups++;
	subMenu = groupMenu;
      }
    }
    QAction *action=subMenu->addAction(itemName.c_str());
    QObject::connect(action,SIGNAL(triggered()),receiver,slot);
  }
  //**********************************************************************
  template <typename TYPEN, typename TYPEE, typename TPROPERTY>
  void buildPropertyMenu(QMenu &menu, QObject *receiver, const char *slot) {
    typename TemplateFactory<PropertyFactory<TPROPERTY>, TPROPERTY, PropertyContext>::ObjectCreator::const_iterator it;
    std::vector<QMenu*> groupMenus;
    std::string::size_type nGroups = 0;
    it=AbstractProperty<TYPEN, TYPEE, TPROPERTY>::factory->objMap.begin();
    for (;it!=AbstractProperty<TYPEN,TYPEE, TPROPERTY>::factory->objMap.end();++it)
      insertInMenu(menu, it->first.c_str(), it->second->getGroup(), groupMenus, nGroups,receiver,slot);
  }
  template <typename TFACTORY, typename TMODULE>
  void buildMenuWithContext(QMenu &menu, QObject *receiver, const char *slot) {
    typename TemplateFactory<TFACTORY, TMODULE, AlgorithmContext>::ObjectCreator::const_iterator it;
    std::vector<QMenu*> groupMenus;
    std::string::size_type nGroups = 0;
    for (it=TFACTORY::factory->objMap.begin();it != TFACTORY::factory->objMap.end();++it)
      insertInMenu(menu, it->first.c_str(), it->second->getGroup(), groupMenus, nGroups,receiver,slot);
  }
  typedef std::vector<node> NodeA;
  typedef std::vector<edge> EdgeA;

  void GetSelection(NodeA & outNodeA, EdgeA & outEdgeA,
		    Graph *inG, BooleanProperty * inSel ) {
    assert( inSel );
    assert( inG );
    outNodeA.clear();
    outEdgeA.clear();
    // Get edges
    Iterator<edge> * edgeIt = inG->getEdges();
    while( edgeIt->hasNext() ) {
      edge e = edgeIt->next();
      if( inSel->getEdgeValue(e) )
	outEdgeA.push_back( e );
    } delete edgeIt;
    // Get nodes
    Iterator<node> * nodeIt = inG->getNodes();
    while( nodeIt->hasNext() ) {
      node n = nodeIt->next();
      if( inSel->getNodeValue(n) )
	outNodeA.push_back( n );
    } delete nodeIt;
  }

  void SetSelection(BooleanProperty * outSel, NodeA & inNodeA,
		    EdgeA & inEdgeA, Graph * inG) {
    assert( outSel );
    assert( inG );
    outSel->setAllNodeValue( false );
    outSel->setAllEdgeValue( false );
    // Set edges
    for( unsigned int e = 0 ; e < inEdgeA.size() ; e++ )
      outSel->setEdgeValue( inEdgeA[e], true );
    // Set nodes
    for( unsigned int n = 0 ; n < inNodeA.size() ; n++ )
      outSel->setNodeValue( inNodeA[n], true );
  }
  //==================================================
  #define UNNAMED "unnamed"
  std::string newName() {
    static int idx = 0;

    if (idx++ == 0)
      return std::string(UNNAMED);
    stringstream ss;
    ss << UNNAMED << '_' << idx - 1;
    return ss.str();
  }


  //**********************************************************************
  //**********************************************************************
  //**********************************************************************
  //**********************************************************************
  //**********************************************************************
#ifndef WITHOUT_MAIN_CONTROLLER
  CONTROLLERPLUGIN(MainController, "MainController", "Tulip Team", "16/04/2008", "Main controller", "1.0");
#endif

  //**********************************************************************
  MainController::MainController():
    copyCutPasteGraph(NULL),currentGraphNbNodes(0),currentGraphNbEdges(0),graphToReload(NULL),blockUpdate(false),clusterTreeWidget(NULL) {
    morph = new Morphing();
  }
  //**********************************************************************
  MainController::~MainController() {
    clearObservers();
    Graph *currentGraph=getGraph();
    if (currentGraph) {
      currentGraph->removeObserver(this);
      currentGraph->removeGraphObserver(this);
      delete editMenu;
      delete algorithmMenu;
      delete viewMenu;
      delete optionsMenu;
      delete graphMenu;
      delete undoAction;
      delete redoAction;

      delete clusterTreeWidget;
      delete propertiesWidget;
      delete eltProperties;

      delete tabWidgetDock;
    }
  }
  //**********************************************************************
  void MainController::attachMainWindow(MainWindowFacade facade){
    ControllerViewsManager::attachMainWindow(facade);
    loadGUI();
  }
  //**********************************************************************
  // define some specific MetaValueCalculator classes
  // viewColor
  class ViewColorCalculator :public AbstractColorProperty::MetaValueCalculator {
  public:
    virtual void computeMetaValue(AbstractColorProperty* color, node mN,
				  Graph* sg, Graph*) {
      // meta node color is half opaque white
      color->setNodeValue(mN, Color(255, 255, 255, 127));
    }

    virtual void computeMetaValue(AbstractColorProperty* color, edge mE,
				  Iterator<edge>*itE, Graph*) {
      // meta edge color is the color of the first underlying edge
      color->setEdgeValue(mE, color->getEdgeValue(itE->next()));
    }
  };

  // viewLabel
  class ViewLabelCalculator :public AbstractStringProperty::MetaValueCalculator {
  public:
    // set the meta node label to label of viewMetric max corresponding node
    void computeMetaValue(AbstractStringProperty* label,
			  node mN, Graph* sg, Graph*) {
      // nothing to do if viewMetric does not exist
      if (!sg->existProperty("viewMetric"))
	return;
      node viewMetricMaxNode;
      double vMax = -DBL_MAX;
      DoubleProperty *metric = sg->getProperty<DoubleProperty>("viewMetric");
      Iterator<node> *itN= sg->getNodes();
      while (itN->hasNext()){
	node itn = itN->next();
	const double& value = metric->getNodeValue(itn);
	if (value > vMax) {
	  vMax = value;
	  viewMetricMaxNode = itn;
	}
      } delete itN;
      label->setNodeValue(mN, label->getNodeValue(viewMetricMaxNode));
    }
  };

  // viewLayout
  class ViewLayoutCalculator :public AbstractLayoutProperty::MetaValueCalculator {
  public:
    void computeMetaValue(AbstractLayoutProperty* layout,
			  node mN, Graph* sg, Graph* mg) {
      SizeProperty* size = mg->getProperty<SizeProperty>("viewSize");
      DoubleProperty* rot = mg->getProperty<DoubleProperty>("viewRotation");
      std::pair<Coord, Coord> box =
      tlp::computeBoundingBox(sg, (LayoutProperty *) layout, size, rot);
      Coord maxL = box.first;
      Coord minL = box.second;
      layout->setNodeValue(mN, (maxL + minL) / 2.0 );
      Coord v = (maxL - minL);
      if (v[2] < 0.0001) v[2] = 0.1;
      mg->getProperty<SizeProperty>("viewSize")->
	setNodeValue(mN, Size(v[0],v[1],v[2]));
    }
  };

  class ViewSizeCalculator
    :public AbstractSizeProperty::MetaValueCalculator {
  public:
    void computeMetaValue(AbstractSizeProperty*, node, Graph*, Graph*) {
      // do nothing
    }
  };

  // corresponding static instances
  static ViewColorCalculator vColorCalc;
  static ViewLabelCalculator vLabelCalc;
  static ViewLayoutCalculator vLayoutCalc;
  static ViewSizeCalculator vSizeCalc;
  //**********************************************************************
  void MainController::setData(Graph *graph,DataSet dataSet) {
    editMenu->setEnabled(true);
    algorithmMenu->setEnabled(true);
    viewMenu->setEnabled(true);
    optionsMenu->setEnabled(true);
    graphMenu->setEnabled(true);

    Observable::holdObservers();
    Graph *newGraph=graph;
    newGraph->addObserver(this);
    newGraph->addGraphObserver(this);
    Graph *lastViewedGraph=newGraph;
    Observable::unholdObservers();
    setCurrentGraph(newGraph);
    // install predefined meta value calculators
    newGraph->getProperty<ColorProperty>("viewColor")->
      setMetaValueCalculator(&vColorCalc);
    newGraph->getProperty<StringProperty>("viewLabel")->
      setMetaValueCalculator(&vLabelCalc);
    newGraph->getProperty<LayoutProperty>("viewLayout")->
      setMetaValueCalculator(&vLayoutCalc);
    newGraph->getProperty<SizeProperty>("viewSize")->
      setMetaValueCalculator(&vSizeCalc);
    if(dataSet.exist("views")) {
      DataSet views;
      dataSet.get<DataSet>("views", views);
      Iterator< std::pair<std::string, DataType*> > *it=views.getValues();
      if(!it->hasNext()){
        initMainView(DataSet());
      }else{
        while(it->hasNext()) {
          pair<string, DataType*> p;
          p = it->next();
          Iterator< std::pair<std::string, DataType*> > *it2=(*(DataSet*)p.second->value).getValues();
          pair<string, DataType*> v=it2->next();
          int x,y,width,height;

          if((*(DataSet*)p.second->value).exist("id")){
            int id;
            (*(DataSet*)p.second->value).get("id",id);
            if(id!=0){
	      lastViewedGraph=newGraph->getDescendantGraph(id);
              if(!lastViewedGraph)
                lastViewedGraph=newGraph;
            }
          }

          if((*(DataSet*)p.second->value).exist("graphName")){
            string graphName;
            (*(DataSet*)p.second->value).get("graphName",graphName);
            lastViewedGraph=getCurrentSubGraph(newGraph, graphName);
            if(!lastViewedGraph)
              lastViewedGraph=newGraph;
          }

          (*(DataSet*)p.second->value).get("x",x);
          (*(DataSet*)p.second->value).get("y",y);
          (*(DataSet*)p.second->value).get("width",width);
          (*(DataSet*)p.second->value).get("height",height);

          bool maximized=false;
          if((*(DataSet*)p.second->value).exist("maximized")){
            (*(DataSet*)p.second->value).get("maximized",maximized);
          }

          createView(v.first,lastViewedGraph,*(DataSet*)v.second->value,true,QRect(x,y,width,height),maximized);

        }
      }
    }else{
      NodeLinkDiagramComponent *view;
      if(dataSet.exist("scene")) {
        view=(NodeLinkDiagramComponent*)initMainView(dataSet);
      }else{
        view=(NodeLinkDiagramComponent*)initMainView(DataSet());
      }

      if(dataSet.exist("displaying")) {
        GlMainWidget *glW=view->getGlMainWidget();
        GlGraphRenderingParameters param = glW->getScene()->getGlGraphComposite()->getRenderingParameters();
        DataSet displayingData;
        dataSet.get<DataSet>("displaying", displayingData);

        param.setParameters(displayingData);
        glW->getScene()->getGlGraphComposite()->setRenderingParameters(param);
        if(displayingData.exist("backgroundColor")){
          Color backgroundColor;
          displayingData.get<Color>("backgroundColor",backgroundColor);
          glW->getScene()->setBackgroundColor(backgroundColor);
        }
        if(displayingData.exist("cameraEyes") && displayingData.exist("cameraCenter") && displayingData.exist("cameraUp") && displayingData.exist("cameraZoomFactor") && displayingData.exist("distCam")){
          Coord cameraEyes, cameraCenter, cameraUp;
          double cameraZoomFactor, distCam;
          displayingData.get<Coord>("cameraEyes",cameraEyes);
          displayingData.get<Coord>("cameraCenter",cameraCenter);
          displayingData.get<Coord>("cameraUp",cameraUp);
          displayingData.get<double>("cameraZoomFactor",cameraZoomFactor);
          displayingData.get<double>("distCam",distCam);
          Camera *camera=glW->getScene()->getLayer("Main")->getCamera();
          camera->setEyes(cameraEyes);
          camera->setCenter(cameraCenter);
          camera->setUp(cameraUp);
          camera->setZoomFactor(cameraZoomFactor);
          camera->setSceneRadius(distCam);
        }
        // show current subgraph if any
        int id = 0;
        if (displayingData.get<int>("SupergraphId", id) && id) {
          Graph *subGraph = newGraph->getDescendantGraph(id);
          if (subGraph){
            setGraphOfView(view,subGraph);
          }
        }
      }

    }

    clusterTreeWidget->setGraph(lastViewedGraph);
    eltProperties->setGraph(lastViewedGraph);
    propertiesWidget->setGraph(lastViewedGraph);
    updateCurrentGraphInfos();
    initObservers();
  }
  //**********************************************************************
  void MainController::getData(Graph **graph,DataSet *dataSet) {
    DataSet views;
    QWidgetList widgetList;

    widgetList=mainWindowFacade.getWorkspace()->windowList();


    for(int i=0;i<widgetList.size();++i) {
      QRect rect=((QWidget *)(widgetList[i]->parent()))->geometry();
      DataSet tmp;
      stringstream str;
      str << "view" << i ;
      DataSet viewData;
      Graph *graph;
      View *view = getViewOfWidget(widgetList[i]);
      if(view){
        view->getData(&graph,&viewData);
        tmp.set<DataSet>(getNameOfView(view),viewData);
        tmp.set<unsigned int>("id",graph->getId());
        tmp.set<int>("x",rect.left());
        tmp.set<int>("y",rect.top());
        tmp.set<int>("width",rect.width());
        tmp.set<int>("height",rect.height());
        tmp.set<bool>("maximized", ((QWidget *)(widgetList[i]->parent()))->isMaximized());
        views.set<DataSet>(str.str(),tmp);
      }
    }
    dataSet->set<DataSet>("views",views);

    *graph=getCurrentGraph();
  }
  //**********************************************************************
  void MainController::drawViews(bool init) {
    Observable::holdObservers();

    ControllerViewsManager::drawViews(init);
    
    eltProperties->updateTable();
    propertiesWidget->update();

    Observable::unholdObservers();
  }
  //**********************************************************************
  void MainController::observableDestroyed(Observable *) {
    //cerr << "[WARNING]" << __PRETTY_FUNCTION__ << endl;
  }
  //**********************************************************************
  void MainController::update ( ObserverIterator begin, ObserverIterator end) {
    // block update when we do an undo/redo
    if(blockUpdate)
      return;

    blockUpdate=true;

    if(graphToReload){
      // enter here if a property is add/delete on the graph
      Graph *graph=graphToReload;
      graphToReload=NULL;
      
      updateViewsOfGraph(graph);
      updateViewsOfSubGraphs(graph);
    }else{
      drawViews();
    }

    blockUpdate=false;

    updateCurrentGraphInfos();

    updateUndoRedoInfos();
  }
  //**********************************************************************
  void MainController::initObservers() {
    if (!getCurrentGraph()) 
      return;
    
    // Observe properties of the graph
    Iterator<PropertyInterface*> *it = getCurrentGraph()->getObjectProperties();
    while (it->hasNext()) {
      PropertyInterface* tmp = it->next();
      tmp->addObserver(this);
    } delete it;
  }
  //**********************************************************************
  void MainController::clearObservers() {
    if (!getCurrentGraph()) 
      return;
    
    Iterator<PropertyInterface*> *it =getCurrentGraph()->getObjectProperties();
    while (it->hasNext()) {
      (it->next())->removeObserver(this);
    } delete it;
  }
  //**********************************************************************
  void MainController::addSubGraph(Graph *g, Graph *sg){
    if(getCurrentGraph()!=g)
      return;

    sg->addObserver(this);
    sg->addGraphObserver(this);

    clusterTreeWidget->update();
  }
  //**********************************************************************
  void MainController::delSubGraph(Graph *g, Graph *sg){
    Iterator<Graph *> *itS=sg->getSubGraphs();
    while(itS->hasNext()) {
      Graph *subgraph = itS->next();
      delSubGraph(sg,subgraph);
    }

    sg->removeObserver(this);
    sg->removeGraphObserver(this);

    if(getCurrentGraph()==sg)
      setCurrentGraph(g);
    
    changeGraphOfViews(sg,g);
  }
  //**********************************************************************
  void  MainController::addLocalProperty(Graph *graph, const std::string&){
    graphToReload=graph;

    if(graph==getCurrentGraph()){
      eltProperties->setGraph(graph);
      propertiesWidget->setGraph(graph);
    }
  }
  //**********************************************************************
  void  MainController::delLocalProperty(Graph *graph, const std::string&){
    graphToReload=graph;

    if(graph==getCurrentGraph()){
      eltProperties->setGraph(graph);
      propertiesWidget->setGraph(graph);
    }
  }
  //**********************************************************************
  void MainController::afterSetAttribute(Graph *graph, const std::string &name){
    // In this function we only do threatment if graph name is changed (attribute "name" is changed)
    if(name=="name")
      clusterTreeWidget->update();
  }
  //**********************************************************************
  void MainController::loadGUI() {

  	mainWindowFacade.getWorkspace()->setScrollBarsEnabled( true );

    //+++++++++++++++++++++++++++
    //Create Data information editor (Hierarchy, Element info, Property Info)
    tabWidgetDock = new QDockWidget("Data manipulation", mainWindowFacade.getParentWidget());
    tabWidgetDock->hide();
    tabWidgetDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    tabWidgetDock->setWindowTitle("Graph Editor");
    tabWidgetDock->setFeatures(QDockWidget::DockWidgetClosable |
			    QDockWidget::DockWidgetMovable |
			    QDockWidget::DockWidgetFloatable);
    TabWidget *tabWidget = new TabWidget(tabWidgetDock);
    tabWidgetDock->setWidget(tabWidget);
    mainWindowFacade.addDockWidget(Qt::LeftDockWidgetArea, tabWidgetDock);
    tabWidget->show();
    tabWidgetDock->show();

    //+++++++++++++++++++++++++++
    //Init hierarchy visualization widget
    clusterTreeWidget=tabWidget->clusterTree;
      //connect signals related to graph replacement
    connect(clusterTreeWidget, SIGNAL(graphChanged(Graph *)),this, SLOT(changeGraph(Graph *)));
    connect(clusterTreeWidget, SIGNAL(aboutToRemoveView(Graph *)), this, SLOT(graphAboutToBeRemove(Graph *)));
    //Init Property Editor Widget
    propertiesWidget=tabWidget->propertyDialog;
    propertiesWidget->setGraph(NULL);
    connect(propertiesWidget->tableNodes, SIGNAL(showElementProperties(unsigned int,bool)),
	    this, SLOT(showElementProperties(unsigned int,bool)));
    connect(propertiesWidget->tableEdges, SIGNAL(showElementProperties(unsigned int,bool)),
	    this, SLOT(showElementProperties(unsigned int,bool)));
    //Init Element info widget
    eltProperties = tabWidget->elementInfo;

    configWidgetDock = new QDockWidget("Data manipulation", mainWindowFacade.getParentWidget());
    configWidgetTab = new QTabWidget(configWidgetDock);
    configWidgetTab->setFocusPolicy(Qt::StrongFocus);

    configWidgetTab->addTab(ControllerViewsTools::getNoInteractorConfigurationWidget(),"Interactor");
    configWidgetTab->setTabPosition(QTabWidget::West);
    configWidgetDock->setWidget(configWidgetTab);
    configWidgetDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    configWidgetDock->setWindowTitle("View Editor");
    configWidgetDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    mainWindowFacade.addDockWidget(Qt::LeftDockWidgetArea, configWidgetDock);

    mainWindowFacade.tabifyDockWidget(tabWidgetDock,configWidgetDock);

    buildMenu();

  }
  //**********************************************************************
  void MainController::buildMenu() {
    QAction *tmpAction;

    //Search the Windows menu to add others menu before this menu
    QAction *windowAction=NULL;
    QList<QAction *> menuBarActions=mainWindowFacade.getMenuBar()->actions();
    for(QList<QAction *>::iterator it=menuBarActions.begin();it!=menuBarActions.end();++it) {
      if((*it)->text()=="&Windows")
        windowAction=*it;
    }
    assert(windowAction);

    editMenu = new QMenu("&Edit");
    editMenu->setEnabled(false);
    mainWindowFacade.getMenuBar()->insertMenu(windowAction,editMenu);

    tmpAction=editMenu->addAction("&Cut",this,SLOT(editCut()),QKeySequence(tr("Ctrl+X")));
    editMenu->addAction("C&opy",this,SLOT(editCopy()),QKeySequence(tr("Ctrl+C")));
    editMenu->addAction("&Paste",this,SLOT(editPaste()),QKeySequence(tr("Ctrl+V")));
    editMenu->addSeparator();
    editMenu->addAction("&Find...",this,SLOT(editFind()),QKeySequence(tr("Ctrl+F")));
    editMenu->addSeparator();
    editMenu->addAction("Select all",this,SLOT(editSelectAll()),QKeySequence(tr("Ctrl+A")));
    editMenu->addAction("Delete selection",this,SLOT(editDelSelection()),QKeySequence(tr("Del")));
    editMenu->addAction("Deselect all",this,SLOT(editDeselectAll()),QKeySequence(tr("Ctrl+Shift+A")));
    editMenu->addAction("Invert selection",this,SLOT(editReverseSelection()),QKeySequence(tr("Ctrl+I")));
    editMenu->addSeparator();
    editMenu->addAction("Create group",this,SLOT(editCreateGroup()),QKeySequence(tr("Ctrl+G")));
    editMenu->addAction("Create subgraph",this,SLOT(editCreateSubgraph()),QKeySequence(tr("Ctrl+Shift+G")));
    editMenu->addSeparator();
    editUndoAction=editMenu->addAction("Undo",this,SLOT(undo()),QKeySequence(tr("Ctrl+Z")));
    editUndoAction->setEnabled(false);
    editRedoAction=editMenu->addAction("Redo",this,SLOT(redo()),QKeySequence(tr("Ctrl+Y")));
    editRedoAction->setEnabled(false);

     //Algorithm Menu
    algorithmMenu = new QMenu("&Algorithm");
    algorithmMenu->setEnabled(false);
    intMenu=new QMenu("&Integer");
    stringMenu=new QMenu("L&abel");
    sizesMenu=new QMenu("S&ize");
    colorsMenu=new QMenu("&Color");
    layoutMenu=new QMenu("&Layout");
    metricMenu=new QMenu("&Measure");
    selectMenu=new QMenu("&Selection");
    generalMenu=new QMenu("&General");

    buildPropertyMenu<IntegerType, IntegerType, IntegerAlgorithm>(*intMenu, this, SLOT(changeInt()));
    buildPropertyMenu<StringType, StringType, StringAlgorithm>(*stringMenu, this, SLOT(changeString()));
    buildPropertyMenu<SizeType, SizeType, SizeAlgorithm>(*sizesMenu, this, SLOT(changeSizes()));
    buildPropertyMenu<ColorType, ColorType, ColorAlgorithm>(*colorsMenu, this, SLOT(changeColors()));
    buildPropertyMenu<PointType, LineType, LayoutAlgorithm>(*layoutMenu, this, SLOT(changeLayout()));
    buildPropertyMenu<DoubleType, DoubleType, DoubleAlgorithm>(*metricMenu, this, SLOT(changeMetric()));
    buildPropertyMenu<BooleanType, BooleanType, BooleanAlgorithm>(*selectMenu, this, SLOT(changeSelection()));
    buildMenuWithContext<AlgorithmFactory, Algorithm>(*generalMenu, this, SLOT(applyAlgorithm()));

    if (selectMenu->actions().count()>0)
      algorithmMenu->addMenu(selectMenu);
    if (colorsMenu->actions().count()>0)
      algorithmMenu->addMenu(colorsMenu);
    if (metricMenu->actions().count()>0)
      algorithmMenu->addMenu(metricMenu);
    if (intMenu->actions().count()>0)
      algorithmMenu->addMenu(intMenu);
    if (layoutMenu->actions().count()>0)
      algorithmMenu->addMenu(layoutMenu);
    if (sizesMenu->actions().count()>0)
      algorithmMenu->addMenu(sizesMenu);
    if (stringMenu->actions().count()>0)
      algorithmMenu->addMenu(stringMenu);
    if (generalMenu->actions().count()>0)
      algorithmMenu->addMenu(generalMenu);
    mainWindowFacade.getMenuBar()->insertMenu(windowAction,algorithmMenu);


    //Graph menu
    graphMenu = new QMenu("&Graph");
    graphMenu->setEnabled(false);
    QMenu *testGraphMenu=graphMenu->addMenu("Test");
    tmpAction=testGraphMenu->addAction("Simple");
    connect(tmpAction,SIGNAL(triggered()),this,SLOT(isSimple()));
    tmpAction=testGraphMenu->addAction("Directed Tree");
    connect(tmpAction,SIGNAL(triggered()),this,SLOT(isTree()));
    tmpAction=testGraphMenu->addAction("Free Tree");
    connect(tmpAction,SIGNAL(triggered()),this,SLOT(isFreeTree()));
    tmpAction=testGraphMenu->addAction("Acyclic");
    connect(tmpAction,SIGNAL(triggered()),this,SLOT(isAcyclic()));
    tmpAction=testGraphMenu->addAction("Connected");
    connect(tmpAction,SIGNAL(triggered()),this,SLOT(isConnected()));
    tmpAction=testGraphMenu->addAction("Biconnected");
    connect(tmpAction,SIGNAL(triggered()),this,SLOT(isBiconnected()));
    tmpAction=testGraphMenu->addAction("Triconnected");
    connect(tmpAction,SIGNAL(triggered()),this,SLOT(isTriconnected()));
    tmpAction=testGraphMenu->addAction("Planar");
    connect(tmpAction,SIGNAL(triggered()),this,SLOT(isPlanar()));
    tmpAction=testGraphMenu->addAction("Outer Planar");
    connect(tmpAction,SIGNAL(triggered()),this,SLOT(isOuterPlanar()));

    QMenu *modifyGraphMenu=graphMenu->addMenu("&Modify");
    tmpAction=modifyGraphMenu->addAction("Make simple");
    connect(tmpAction,SIGNAL(triggered()),this,SLOT(makeSimple()));
    tmpAction=modifyGraphMenu->addAction("Make acyclic");
    connect(tmpAction,SIGNAL(triggered()),this,SLOT(makeAcyclic()));
    tmpAction=modifyGraphMenu->addAction("Make connected");
    connect(tmpAction,SIGNAL(triggered()),this,SLOT(makeConnected()));
    tmpAction=modifyGraphMenu->addAction("Make biconnected");
    connect(tmpAction,SIGNAL(triggered()),this,SLOT(makeBiconnected()));
    tmpAction=modifyGraphMenu->addAction("Make directed");
    connect(tmpAction,SIGNAL(triggered()),this,SLOT(makeDirected()));
    modifyGraphMenu->addSeparator();
    tmpAction=modifyGraphMenu->addAction("Reverse selected edges");
    connect(tmpAction,SIGNAL(triggered()),this,SLOT(reverseSelectedEdgeDirection()));

    mainWindowFacade.getMenuBar()->insertMenu(windowAction,graphMenu);


    //View menu
    viewMenu = new QMenu("View");
    viewMenu->setEnabled(false);
    connect(viewMenu, SIGNAL(triggered(QAction *)), SLOT(createView(QAction*)));
    TemplateFactory<ViewFactory, View, ViewContext>::ObjectCreator::const_iterator it;
    for (it=ViewFactory::factory->objMap.begin();it != ViewFactory::factory->objMap.end();++it) {
      viewMenu->addAction(it->first.c_str());
    }
    mainWindowFacade.getMenuBar()->insertMenu(windowAction,viewMenu);

    //Options menu
    optionsMenu = new QMenu("&Options");
    optionsMenu->setEnabled(false);
    forceRatioAction = optionsMenu->addAction("Force ratio");
    forceRatioAction->setCheckable(true);
    forceRatioAction->setChecked(false);
    mapMetricAction = optionsMenu->addAction("Map metric");
    mapMetricAction->setCheckable(true);
    mapMetricAction->setChecked(true);
    morphingAction = optionsMenu->addAction("Morphing");
    morphingAction->setCheckable(true);
    morphingAction->setChecked(false);
    optionsMenu->addSeparator();
    QAction *propertiesDockAction = optionsMenu->addAction("Show graph editor");
    connect(propertiesDockAction,SIGNAL(triggered()),tabWidgetDock,SLOT(show()));
    QAction *configurationDockAction = optionsMenu->addAction("Show view editor");
    connect(configurationDockAction,SIGNAL(triggered()),configWidgetDock,SLOT(show()));
    mainWindowFacade.getMenuBar()->insertMenu(windowAction,optionsMenu);

    redoAction=new QAction(QIcon(":/i_redo.png"),"redo",mainWindowFacade.getParentWidget());
    undoAction=new QAction(QIcon(":/i_undo.png"),"undo",mainWindowFacade.getParentWidget());
    undoAction->setEnabled(false);
    redoAction->setEnabled(false);
    mainWindowFacade.getToolBar()->addAction(undoAction);
    mainWindowFacade.getToolBar()->addAction(redoAction);
    connect(undoAction,SIGNAL(triggered()),this,SLOT(undo()));
    connect(redoAction,SIGNAL(triggered()),this,SLOT(redo()));
  }
  //**********************************************************************
  View* MainController::initMainView(DataSet dataSet) {
    View* newView=createView("Node Link Diagram view",getCurrentGraph(),dataSet);
    return newView;
  }
  //**********************************************************************
  View* MainController::createView(const string &name,Graph *graph,DataSet dataSet,bool forceWidgetSize,const QRect &rect,bool maximized){ 
    QRect newRect=rect;
    forceWidgetSize=true;
    unsigned int viewsNumber=getViewsNumber();
    if(newRect.width()==0 && newRect.height()==0){
      forceWidgetSize=false;
      newRect=QRect(QPoint((viewsNumber)*20,(viewsNumber)*20),QSize(0,0));
    }
    
    View *createdView=ControllerViewsManager::createView(name,graph,dataSet,forceWidgetSize,newRect,maximized);

    connect(createdView, SIGNAL(elementSelected(unsigned int, bool)),this,SLOT(showElementProperties(unsigned int, bool)));
    connect(createdView, SIGNAL(requestChangeGraph(View *,Graph *)), this, SLOT(viewRequestChangeGraph(View *,Graph *)));

    return createdView;
  }
  //**********************************************************************
  bool MainController::windowActivated(QWidget *widget) {
    
    lastConfigTabIndexOnView[getCurrentView()]=configWidgetTab->currentIndex();
    
    if(!ControllerViewsManager::windowActivated(widget))
      return false;
    
    // Remove tabs of View Editor
    while(configWidgetTab->count()>0){
      configWidgetTab->removeTab(0);
    }
    
    // Find view and graph of this widget
    View *view=getViewOfWidget(widget);
    Graph *graph=getGraphOfView(view);
    
    // Update left part of tulip
    clusterTreeWidget->setGraph(graph);
    eltProperties->setGraph(graph);
    propertiesWidget->setGraph(graph);

    // Load interactor configuration widget
    QWidget *interactorConfigurationWidget=ControllerViewsManager::getInteractorConfigurationWidgetOfView(view);
    if(interactorConfigurationWidget)
      configWidgetTab->addTab(interactorConfigurationWidget,"Interactor");
    else
      configWidgetTab->addTab(ControllerViewsTools::getNoInteractorConfigurationWidget(),"Interactor");

    // Load view configuration widget
    list<pair<QWidget *,string> > configWidgetsList=view->getConfigurationWidget();
    for(list<pair<QWidget *,string> >::iterator it=configWidgetsList.begin();it!=configWidgetsList.end();++it){
      configWidgetTab->addTab((*it).first,(*it).second.c_str());
    }
    if(lastConfigTabIndexOnView.count(view)!=0)
      configWidgetTab->setCurrentIndex(lastConfigTabIndexOnView[view]);

    //Remove observer (nothing if this not observe)
    graph->removeGraphObserver(this);
    graph->removeObserver(this);
    //Add observer
    graph->addGraphObserver(this);
    graph->addObserver(this);
    
    return true;
  }
  //**********************************************************************
  bool MainController::changeGraph(Graph *graph) {
    if(getCurrentGraph()==graph)
      return false;
    if(!getCurrentView())
      return false;

    clearObservers();

    ControllerViewsManager::changeGraph(graph);

    clusterTreeWidget->setGraph(graph);
    eltProperties->setGraph(graph);
    propertiesWidget->setGraph(graph);

    updateCurrentGraphInfos();
    updateUndoRedoInfos();
    
    initObservers();
    //Remove observer (nothing if this not observe)
    graph->removeGraphObserver(this);
    graph->removeObserver(this);
    //Add observer
    graph->addGraphObserver(this);
    graph->addObserver(this);
    // install predefined meta value calculators
    graph->getProperty<ColorProperty>("viewColor")->
      setMetaValueCalculator(&vColorCalc);
    graph->getProperty<StringProperty>("viewLabel")->
      setMetaValueCalculator(&vLabelCalc);
    graph->getProperty<LayoutProperty>("viewLayout")->
      setMetaValueCalculator(&vLayoutCalc);
    graph->getProperty<SizeProperty>("viewSize")->
      setMetaValueCalculator(&vSizeCalc);
    
    return true;
  }
  //**********************************************************************
   void MainController::graphAboutToBeRemove(Graph *graph){
     setCurrentGraph(NULL);
   }
  //**********************************************************************
  bool MainController::changeInteractor(QAction* action) {
    QWidget *configurationWidget;
    if(!ControllerViewsManager::changeInteractor(action,&configurationWidget))
      return false;
    
    bool onInteractorConfigTab=configWidgetTab->currentIndex()==0;
    configWidgetTab->removeTab(0);
    configWidgetTab->insertTab(0,configurationWidget,"Interactor");
      
    if(onInteractorConfigTab)
      configWidgetTab->setCurrentIndex(0);

    return true;
  }
  //**********************************************************************
  void MainController::widgetWillBeClosed(QObject *object) {
    ControllerViewsManager::widgetWillBeClosed(object);

    //If after close this widget we have no widget open : clear
    if(getViewsNumber()==0){
      while(configWidgetTab->count()>0)
        configWidgetTab->removeTab(0);
      configWidgetTab->addTab(ControllerViewsTools::getNoInteractorConfigurationWidget(),"Interactor");
    }
  }
  //==================================================
  void MainController::showElementProperties(unsigned int eltId, bool isNode) {
    if (isNode)
      eltProperties->setCurrentNode(getCurrentGraph(),  tlp::node(eltId));
    else
      eltProperties->setCurrentEdge(getCurrentGraph(),  tlp::edge(eltId));
    // show 'Element' tab in 'Info Editor'
    QWidget *tab = eltProperties->parentWidget();
    QTabWidget *tabWidget = (QTabWidget *) tab->parentWidget()->parentWidget();
    tabWidget->setCurrentIndex(tabWidget->indexOf(tab));
  }
  //==================================================
  void MainController::viewRequestChangeGraph(View *view,Graph *graph) {
    assert(view==getCurrentView());
    changeGraph(graph);
  }
  //**********************************************************************
  void MainController::updateCurrentGraphInfos() {
    if(!getCurrentGraph())
      return;
    
    static QLabel *currentGraphInfosLabel = 0;
    if (!currentGraphInfosLabel) {
      currentGraphInfosLabel = new QLabel(mainWindowFacade.getStatusBar());
      mainWindowFacade.getStatusBar()->addPermanentWidget(currentGraphInfosLabel);
    }

    currentGraphNbNodes=getCurrentGraph()->numberOfNodes();
    currentGraphNbEdges=getCurrentGraph()->numberOfEdges();
    
    char tmp[255];
    sprintf(tmp,"nodes:%d, edges:%d", currentGraphNbNodes, currentGraphNbEdges);
    currentGraphInfosLabel->setText(tmp);
    // Update nb nodes/edges for current graph ans sub graphs
    clusterTreeWidget->updateCurrentGraphInfos(getCurrentGraph());
  }
  //==============================================================
  void MainController::editCut() {
    Graph *graph=getCurrentGraph();
    if(!graph)
    	return;
    
    // free the previous ccpGraph
    if( copyCutPasteGraph ) {
      delete copyCutPasteGraph;
      copyCutPasteGraph = 0;
    }
    BooleanProperty * selP = graph->getProperty<BooleanProperty>("viewSelection");
    if( !selP ) return;
    // Save selection
    NodeA nodeA;
    EdgeA edgeA;
    GetSelection( nodeA, edgeA, graph, selP );
    Observable::holdObservers();
    Graph* newGraph = tlp::newGraph();
    tlp::copyToGraph( newGraph, graph, selP );
    stringstream tmpss;
    DataSet dataSet;
    tlp::exportGraph(newGraph, tmpss, "tlp", dataSet, NULL);
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(tmpss.str().c_str());
    graph->push();
    // Restore selection
    SetSelection( selP, nodeA, edgeA, graph );
    tlp::removeFromGraph( graph, selP );
    Observable::unholdObservers();
    drawViews();
  }
  //==============================================================
  void MainController::editCopy() {
    Graph *graph=getCurrentGraph();
    if(!graph)
      return;
   
    // free the previous ccpGraph
    if( copyCutPasteGraph ) {
      delete copyCutPasteGraph;
      copyCutPasteGraph = 0;
    }
    BooleanProperty * selP = graph->getProperty<BooleanProperty>("viewSelection");
    if( !selP ) return;
    Observable::holdObservers();
    Graph* newGraph = tlp::newGraph();
    tlp::copyToGraph( newGraph, graph, selP );
    stringstream tmpss;
    DataSet dataSet;
    tlp::exportGraph(newGraph, tmpss, "tlp", dataSet, NULL);
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(QString::fromUtf8(tmpss.str().c_str()));
    Observable::unholdObservers();
  }
  //==============================================================
  void MainController::editPaste() {
    Graph *graph=getCurrentGraph();
    if(!graph)
      return;
    
    graph->removeObserver(this);
    Observable::holdObservers();
    BooleanProperty * selP = graph->getProperty<BooleanProperty>("viewSelection");

    graph->push();
    Graph *newGraph=tlp::newGraph();
    DataSet dataSet;
    QClipboard *clipboard = QApplication::clipboard();
    dataSet.set<string>("file::data", clipboard->text().toUtf8().data());
    tlp::importGraph("tlp", dataSet, NULL ,newGraph);
    tlp::copyToGraph( graph, newGraph, 0, selP );
    Observable::unholdObservers();
    graph->addObserver(this);
    
    updateCurrentGraphInfos();

    drawViews(true);
  }
  //==============================================================
  void MainController::editFind() {
    Graph *graph=getCurrentGraph();
    if(!graph)
      return;

    static string currentProperty;
    FindSelectionWidget *sel = new FindSelectionWidget(graph, currentProperty, mainWindowFacade.getParentWidget());
    Observable::holdObservers();
    // allow to undo
    graph->push();
    int nbItemsFound = sel->exec();
    if (nbItemsFound > - 1)
      currentProperty = sel->getCurrentProperty();
    delete sel;
    switch(nbItemsFound) {
    case 0:
      mainWindowFacade.getStatusBar()->showMessage("No item found.");
    case -1:
      // forget the current graph state
      graph->pop(false);
      break;
    default:
      stringstream sstr;
      sstr << nbItemsFound << " item(s) found.";
      mainWindowFacade.getStatusBar()->showMessage(sstr.str().c_str());
    }
    // unhold at last to ensure that in case of cancellation or
    // no item found the current graph state has been pop before
    // the call to updateUndoRedoInfos
    Observable::unholdObservers();
  }
  //==============================================================
  void MainController::editCreateGroup() {
    Graph *graph=getCurrentGraph();
    if(!graph)
      return;
    
    set<node> tmp;
    Iterator<node> *it=graph->getNodes();
    BooleanProperty *select = graph->getProperty<BooleanProperty>("viewSelection");
    while (it->hasNext()) {
      node itn = it->next();
      if (select->getNodeValue(itn))
        tmp.insert(itn);
    }delete it;
    if (tmp.empty()) return;
    graph->push();
    Observable::holdObservers();
    bool haveToChangeGraph=false;
    Graph *graphToAddTo=graph;
    if (graphToAddTo == graphToAddTo->getRoot()) {
      QMessageBox::critical( 0, "Warning" ,"Grouping can't be done on the root graph, a subgraph will be created");
      graphToAddTo = tlp::newCloneSubGraph(graphToAddTo, "groups");
      haveToChangeGraph=true;
    }
    node metaNode = graphToAddTo->createMetaNode(tmp);
    if(haveToChangeGraph)
      changeGraph(graphToAddTo);
    Observable::unholdObservers();
    clusterTreeWidget->update();
  }
  //==============================================================
  void MainController::editCreateSubgraph() {
    Graph *graph=getCurrentGraph();
    if(!graph)
      return;
    
    bool ok = FALSE;
    string tmp;
    bool verifGraph = true;
    BooleanProperty *sel1 = graph->getProperty<BooleanProperty>("viewSelection");
    Observable::holdObservers();
    Iterator<edge>*itE = graph->getEdges();
    while (itE->hasNext()) {
      edge ite= itE->next();
      if (sel1->getEdgeValue(ite)) {
        if (!sel1->getNodeValue(graph->source(ite))) {sel1->setNodeValue(graph->source(ite),true); verifGraph=false;}
        if (!sel1->getNodeValue(graph->target(ite))) {sel1->setNodeValue(graph->target(ite),true); verifGraph=false;}
      }
    } delete itE;
    Observable::unholdObservers();

    if(!verifGraph)
      QMessageBox::critical( 0, "Tulip Warning" ,"The selection wasn't a graph, missing nodes have been added");
    QString text = QInputDialog::getText(mainWindowFacade.getParentWidget(),
        "Creation of subgraph" ,
        "Please enter the subgraph name" ,
        QLineEdit::Normal, QString::null, &ok);
    if (ok && !text.isEmpty()) {
      sel1 = graph->getProperty<BooleanProperty>("viewSelection");
      graph->push();
      Graph *tmp = graph->addSubGraph(sel1);
      tmp->setAttribute("name", string(text.toUtf8().data()));
      clusterTreeWidget->update();
      //emit clusterTreeNeedUpdate();
    }
    else if (ok) {
      sel1 = graph->getProperty<BooleanProperty>("viewSelection");
      graph->push();
      Graph *tmp=graph->addSubGraph(sel1);
      tmp->setAttribute("name", newName());
      clusterTreeWidget->update();
      //emit clusterTreeNeedUpdate();
    }
  }
  //==============================================================
  void MainController::editDelSelection() {
    Graph *graph=getCurrentGraph();
    if(!graph)
      return;
    
    graph->push();
    graph->removeObserver(this);
    Observable::holdObservers();
    BooleanProperty *elementSelected=graph->getProperty<BooleanProperty>("viewSelection");
    StableIterator<node> itN(graph->getNodes());
    while(itN.hasNext()) {
      node itv = itN.next();
      if (elementSelected->getNodeValue(itv)==true)
        graph->delNode(itv);
    }
    StableIterator<edge> itE(graph->getEdges());
    while(itE.hasNext()) {
      edge ite=itE.next();
      if (elementSelected->getEdgeValue(ite)==true)
        graph->delEdge(ite);
    }
    Observable::unholdObservers();
    graph->addObserver(this);
    updateCurrentGraphInfos();

    drawViews();
  }
  //==============================================================
  void MainController::editReverseSelection() {
    Graph *graph=getCurrentGraph();
    if(!graph)
      return;
    
    graph->push();
    Observable::holdObservers();
    if(graph->existLocalProperty("viewSelection")){
      graph->getProperty<BooleanProperty>("viewSelection")->reverse();
    }else{
      BooleanProperty *selectionProperty=graph->getProperty<BooleanProperty>("viewSelection");
      StableIterator<node> itN(graph->getNodes());
      while(itN.hasNext()) {
        node itv = itN.next();
        selectionProperty->setNodeValue(itv,!selectionProperty->getNodeValue(itv));
      }
      StableIterator<edge> itE(graph->getEdges());
      while(itE.hasNext()) {
        edge ite=itE.next();
        selectionProperty->setEdgeValue(ite,!selectionProperty->getEdgeValue(ite));
      }
    }
    Observable::unholdObservers();
  }
  //==============================================================
  void MainController::editSelectAll() {
    Graph *graph=getCurrentGraph();
    if(!graph)
      return;
    
    graph->push();
    Observable::holdObservers();
    BooleanProperty *selectionProperty =
      graph->getProperty<BooleanProperty>("viewSelection");
    node n;
    forEach(n, graph->getNodes()) {
      selectionProperty->setNodeValue(n, true);
     }
    edge e;
    forEach(e, graph->getEdges()) {
      selectionProperty->setEdgeValue(e, true);
    }
    Observable::unholdObservers();
  }
  //==============================================================
  void MainController::editDeselectAll() {
    Graph *graph=getCurrentGraph();
    if(!graph)
      return;
    
    graph->push();
    Observable::holdObservers();
    if(graph->existLocalProperty("viewSelection")){
      graph->getProperty<BooleanProperty>("viewSelection")->setAllNodeValue(false);
      graph->getProperty<BooleanProperty>("viewSelection")->setAllEdgeValue(false);
    }else {
      BooleanProperty *selectionProperty =
	graph->getProperty<BooleanProperty>("viewSelection");
      node n;
      forEach(n, graph->getNodes()) {
	selectionProperty->setNodeValue(n, false);
      }
      edge e;
      forEach(e, graph->getEdges()) {
	selectionProperty->setEdgeValue(e, false);
      }
    }
    Observable::unholdObservers();
  }
  //**********************************************************************
  /// Apply a general algorithm
  void MainController::applyAlgorithm() {
    QAction *action=(QAction*)(sender());
    Graph *graph=getCurrentGraph();
    if(!graph)
      return;
    
    bool result=ControllerAlgorithmTools::applyAlgorithm(graph,mainWindowFacade.getParentWidget(),action->text().toStdString());
    if(result){
      undoAction->setEnabled(graph->canPop());
      editUndoAction->setEnabled(graph->canPop());
      clusterTreeWidget->update();
      clusterTreeWidget->setGraph(graph);
      drawViews(true);
    }
  }
  //**********************************************************************
  void MainController::afterChangeProperty() {
    undoAction->setEnabled(true);
    editUndoAction->setEnabled(true);          
    propertiesWidget->setGraph(getCurrentGraph());
    drawViews(true);
  }
  //**********************************************************************
  GraphState *MainController::constructGraphState() {
    GlMainView *mainView=dynamic_cast<GlMainView *>(getCurrentView());
    if(mainView)
      return new GraphState(mainView->getGlMainWidget());
    
    return NULL;
  }
  //**********************************************************************
  void MainController::applyMorphing(GraphState *graphState){
    GlMainView *mainView=dynamic_cast<GlMainView *>(getCurrentView());
    clearObservers();
    mainView->getGlMainWidget()->getScene()->centerScene();
    GraphState * g1 = constructGraphState();
    bool morphable = morph->init(mainView->getGlMainWidget(), graphState, g1);
    if( !morphable ) {
      delete g1;
      g1 = 0;
    } else {
      morph->start(mainView->getGlMainWidget());
      graphState = 0;	// state remains in morph data ...
    }
    initObservers();
  }
  //**********************************************************************
  void MainController::changeString() {
    QAction *action=(QAction*)(sender());
    if(ControllerAlgorithmTools::changeString(getCurrentGraph(),mainWindowFacade.getParentWidget(),action->text().toStdString(),"viewLabel",getCurrentView()))
      afterChangeProperty();
  }
  //**********************************************************************
  void MainController::changeSelection() {
    QAction *action=(QAction*)(sender());
    if(ControllerAlgorithmTools::changeBoolean(getCurrentGraph(),mainWindowFacade.getParentWidget(),action->text().toStdString(),"viewSelection",getCurrentView()))
      afterChangeProperty();
  }
  //**********************************************************************
  void MainController::changeMetric() {
    QAction *action=(QAction*)(sender());
    if(ControllerAlgorithmTools::changeMetric(getCurrentGraph(),mainWindowFacade.getParentWidget(),action->text().toStdString(),"viewMetric",getCurrentView(),mapMetricAction->isChecked(),"Metric Mapping","viewColor"))
      afterChangeProperty();
  }
  //**********************************************************************
  void MainController::changeLayout() {
    QAction *action=(QAction*)(sender());
    GraphState * g0=NULL;
    if(morphingAction->isChecked())
      g0=constructGraphState();
    
    bool result = ControllerAlgorithmTools::changeLayout(getCurrentGraph(),mainWindowFacade.getParentWidget(),action->text().toStdString(), "viewLayout",getCurrentView());
    if (result) {
      if( forceRatioAction->isChecked() )
        getCurrentGraph()->getLocalProperty<LayoutProperty>("viewLayout")->perfectAspectRatio();

      if( morphingAction->isChecked() && g0) {
        applyMorphing(g0);
      }
    }
    drawViews(true);
  }
  //**********************************************************************
  void MainController::changeInt() {
    QAction *action=(QAction*)(sender());
    if(ControllerAlgorithmTools::changeInt(getCurrentGraph(),mainWindowFacade.getParentWidget(),action->text().toStdString(), "viewInt",getCurrentView()))
      afterChangeProperty();
  }
  //**********************************************************************
  void MainController::changeColors() {
    QAction *action=(QAction*)(sender());
    GraphState * g0=NULL;
    if(morphingAction->isChecked())
      g0=constructGraphState();
    
    bool result = ControllerAlgorithmTools::changeColors(getCurrentGraph(),mainWindowFacade.getParentWidget(),action->text().toStdString(),"viewColor",getCurrentView());
    if( result ) {
      if( morphingAction->isChecked() && g0) {
        applyMorphing(g0);
      }
      drawViews(true);
    }
    if( g0 )
      delete g0;
  }
  //**********************************************************************
  void MainController::changeSizes() {
    QAction *action=(QAction*)(sender());
    GraphState * g0 = NULL;
    GlMainView *mainView=dynamic_cast<GlMainView *>(getCurrentView());
    if(morphingAction->isChecked())
      g0=constructGraphState();
  
    bool result = ControllerAlgorithmTools::changeSizes(getCurrentGraph(),mainWindowFacade.getParentWidget(),action->text().toStdString(),"viewSize",getCurrentView());
    if( result ) {
      if( morphingAction->isChecked() && g0) {
	applyMorphing(g0);
      }
      drawViews(true);
    }
    if( g0 )
      delete g0;
  }
  //**********************************************************************
  void MainController::isAcyclic() {
    ControllerAlgorithmTools::isAcyclic(getCurrentGraph(),mainWindowFacade.getParentWidget());
  }
  void MainController::makeAcyclic() {
    undoAction->setEnabled(true);
    editUndoAction->setEnabled(true);
    ControllerAlgorithmTools::makeAcyclic(getCurrentGraph());
  }
  //**********************************************************************
  void MainController::isSimple() {
    ControllerAlgorithmTools::isSimple(getCurrentGraph(),mainWindowFacade.getParentWidget());
  }
  void MainController::makeSimple() {
    undoAction->setEnabled(true);
    editUndoAction->setEnabled(true);
    ControllerAlgorithmTools::makeSimple(getCurrentGraph());
  }
  //**********************************************************************
  void MainController::isConnected() {
    ControllerAlgorithmTools::isConnected(getCurrentGraph(),mainWindowFacade.getParentWidget());
  }
  void MainController::makeConnected() {
    undoAction->setEnabled(true);
    editUndoAction->setEnabled(true);
    ControllerAlgorithmTools::makeConnected(getCurrentGraph());
  }
  //**********************************************************************
  void MainController::isBiconnected() {
    ControllerAlgorithmTools::isBiconnected(getCurrentGraph(),mainWindowFacade.getParentWidget());
  }
  void MainController::makeBiconnected() {
    undoAction->setEnabled(true);
    editUndoAction->setEnabled(true);
    ControllerAlgorithmTools::makeBiconnected(getCurrentGraph());
  }
  //**********************************************************************
  void MainController::isTriconnected() {
    ControllerAlgorithmTools::isTriconnected(getCurrentGraph(),mainWindowFacade.getParentWidget());
  }
  //**********************************************************************
  void MainController::isTree() {
    ControllerAlgorithmTools::isTree(getCurrentGraph(),mainWindowFacade.getParentWidget());
  }
  //**********************************************************************
  void MainController::isFreeTree() {
    ControllerAlgorithmTools::isFreeTree(getCurrentGraph(),mainWindowFacade.getParentWidget());
  }
  void MainController::makeDirected() {
    undoAction->setEnabled(true);
    editUndoAction->setEnabled(true);
    ControllerAlgorithmTools::makeDirected(mainWindowFacade.getParentWidget(),getCurrentGraph());
  }
  //**********************************************************************
  void MainController::isPlanar() {
    ControllerAlgorithmTools::isPlanar(getCurrentGraph(),mainWindowFacade.getParentWidget());
  }
  //**********************************************************************
  void MainController::isOuterPlanar() {
    ControllerAlgorithmTools::isOuterPlanar(getCurrentGraph(),mainWindowFacade.getParentWidget());
  }
  //**********************************************************************
  void MainController::reverseSelectedEdgeDirection() {
    Observable::holdObservers();
    getCurrentGraph()->push();
    getCurrentGraph()->getProperty<BooleanProperty>("viewSelection")->reverseEdgeDirection();
    Observable::unholdObservers();
  }
  //**********************************************************************
  void MainController::updateUndoRedoInfos() {
    Graph *graph=getCurrentGraph();
    if(!graph)
      return;

    undoAction->setEnabled(graph->canPop());
    redoAction->setEnabled(graph->canUnpop());
    editUndoAction->setEnabled(graph->canPop());
    editRedoAction->setEnabled(graph->canUnpop());
  }
  //**********************************************************************
  void MainController::undo() {
    saveViewsGraphsHierarchies();

    Graph *root=getCurrentGraph()->getRoot();
    blockUpdate=true;
    root->pop();
    blockUpdate=false;

    checkViewsGraphsHierarchy();
    Graph *newGraph=getGraphOfView(getCurrentView());

    changeGraph(newGraph);
    // force clusterTreeWidget to update
    clusterTreeWidget->update();
    propertiesWidget->setGraph(newGraph);
    eltProperties->setGraph(newGraph,false);

    drawViews(true);
    updateCurrentGraphInfos();
    updateUndoRedoInfos();
  }
  //**********************************************************************
  void MainController::redo() {
    saveViewsGraphsHierarchies();

    Graph* root = getCurrentGraph()->getRoot();
    blockUpdate=true;
    root->unpop();
    blockUpdate=false;

    checkViewsGraphsHierarchy();
    Graph *newGraph=getGraphOfView(getCurrentView());
    changeGraph(newGraph);
    // force clusterTreeWidget to update
    clusterTreeWidget->update();
    propertiesWidget->setGraph(newGraph);
    eltProperties->setGraph(newGraph,false);

    drawViews(true);
    updateCurrentGraphInfos();
    updateUndoRedoInfos();
  }

}

